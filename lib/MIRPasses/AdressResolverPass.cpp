#include "MIRPasses/AdressResolverPass.h"
#include "Targets/MSP430/MSP430Options.h"
#include "Targets/RTTarget.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <limits>

namespace llvm {

char AdressResolverPass::ID = 1;

/// How far forward to search for a re-sync point when an anchor instruction
/// does not byte-match at the current dump cursor (e.g. an inline-asm body or
/// an unhandled expansion sits in between).
static constexpr size_t ResyncWindow = 32;

/// Space-separated lower-case hex of a byte vector (diagnostics). Defined
/// below.
static std::string toHex(const std::vector<uint8_t> &Bytes);

AdressResolverPass::AdressResolverPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

bool AdressResolverPass::doFinalization(Module &M) {
  // Verify-only coverage report over the functions LLTA actually analyses.
  // Gated behind -address-resolver-verbose so default runs are unchanged.
  if (AddressResolverVerbose) {
    double Pct = Cov.CodeMIs ? (100.0 * static_cast<double>(Cov.ResolvedMIs) /
                                static_cast<double>(Cov.CodeMIs))
                             : 100.0;
    outs() << "[addr-resolver] coverage (analysed functions): " << Cov.Functions
           << " function(s) resolved, " << Cov.FunctionsNoDumpEntry
           << " without a dump entry; " << Cov.ResolvedMIs << "/" << Cov.CodeMIs
           << " instructions addressed (" << format("%.2f", Pct) << "%), "
           << Cov.BranchTargets << " branch/call target(s) attached; "
           << Cov.ResyncEvents << " re-sync, " << Cov.MismatchEvents
           << " unrecovered mismatch, " << Cov.LeftoverEvents
           << " leftover-at-end event(s).\n";
  }
  return false;
}

void AdressResolverPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.setPreservesAll();
  AU.addRequired<CallGraphWrapperPass>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

// Mirrors FillMuGraphPass::getStartingFunction so the diagnostic scope matches
// the function LLTA actually treats as the analysis entry.
Function *AdressResolverPass::getStartingFunction(CallGraph &CG) {
  Function *StartingFunction = nullptr;
  unsigned int CurrentNumReferences = UINT_MAX;
  bool SeenNumRefsTwice = false;

  for (auto &CGNode : CG) {
    auto *F = CGNode.second->getFunction();
    if (F == nullptr)
      continue;
    if (!StartFunctionName.empty() &&
        F->getName().compare(StartFunctionName) == 0)
      return F;
    auto NumRef = CGNode.second->getNumReferences();
    if (NumRef < CurrentNumReferences) {
      StartingFunction = F;
      CurrentNumReferences = NumRef;
    } else if (NumRef == CurrentNumReferences) {
      SeenNumRefsTwice = true;
    }
  }
  if (SeenNumRefsTwice && StartFunctionName.empty())
    return nullptr;
  return StartingFunction;
}

// Collects the call-graph-reachable set from the start function. This is a
// superset of the functions on any analysed path, so gating diagnostics on it
// never hides a problem in a function LLTA depends on. If no single start
// function can be determined, LLTA analyses everything, so we do too.
void AdressResolverPass::computeAnalyzedSet(CallGraph &CG) {
  Function *Start = getStartingFunction(CG);
  if (!Start) {
    AnalyzeAll = true;
    return;
  }
  std::vector<Function *> Work;
  Work.push_back(Start);
  while (!Work.empty()) {
    Function *F = Work.back();
    Work.pop_back();
    if (!AnalyzedFunctions.insert(F).second)
      continue;
    CallGraphNode *N = CG[F];
    if (!N)
      continue;
    for (auto &CR : *N) {
      Function *Callee = CR.second->getFunction();
      if (Callee && !AnalyzedFunctions.count(Callee))
        Work.push_back(Callee);
    }
  }
}

bool AdressResolverPass::isAnalyzed(const MachineFunction &F) const {
  if (AnalyzeAll)
    return true;
  return AnalyzedFunctions.count(&F.getFunction()) != 0;
}

bool AdressResolverPass::doInitialization(Module &M) {
  // The ELF is parsed lazily on the first machine function (the disassembler
  // needs the active subtarget, only reachable from a MachineFunction). Here we
  // only handle options that do not need it.

  // Parse and store the FRAM start address (consumed by the memory model).
  if (!FRAMStartAddress.empty()) {
    uint64_t FRAM = 0;
    if (StringRef(FRAMStartAddress).getAsInteger(0, FRAM)) {
      errs() << "[addr-resolver] warning: could not parse -fram-start='"
             << FRAMStartAddress << "' as a number; ignoring.\n";
    } else {
      TAR.setFRAMStart(FRAM);
      if (AddressResolverVerbose)
        outs() << "[addr-resolver] FRAM start set to 0x"
               << Twine::utohexstr(FRAM) << "\n";
    }
  }
  return false;
}

void AdressResolverPass::finishParse() {
  // Collect sorted, unique function entry addresses so we can derive the end of
  // each function as "the next function entry".
  SortedEntryAddrs.clear();
  for (const auto &KV : FunctionEntryAddr)
    SortedEntryAddrs.push_back(KV.second);
  std::sort(SortedEntryAddrs.begin(), SortedEntryAddrs.end());
  SortedEntryAddrs.erase(
      std::unique(SortedEntryAddrs.begin(), SortedEntryAddrs.end()),
      SortedEntryAddrs.end());

  // Derive each data object's size as "next symbol address minus its own
  // address", using the sorted set of all symbol addresses (code+data).
  std::sort(AllSymbolAddrs.begin(), AllSymbolAddrs.end());
  AllSymbolAddrs.erase(
      std::unique(AllSymbolAddrs.begin(), AllSymbolAddrs.end()),
      AllSymbolAddrs.end());
  for (auto &Obj : DataObjects) {
    auto Hi = std::upper_bound(AllSymbolAddrs.begin(), AllSymbolAddrs.end(),
                               Obj.Address);
    Obj.Size = (Hi != AllSymbolAddrs.end()) ? (*Hi - Obj.Address) : 0;
    TAR.addDataObject(Obj);
  }

  if (AddressResolverVerbose) {
    outs() << "[addr-resolver] parsed " << DumpInstructions.size()
           << " instructions, " << FunctionEntryAddr.size()
           << " function entries, " << DataObjects.size() << " data objects\n";
    for (const auto &Obj : DataObjects)
      outs() << "[addr-resolver]   data " << Obj.Name << " @0x"
             << Twine::utohexstr(Obj.Address) << " size " << Obj.Size << " ("
             << Obj.Section << ")\n";
  }
}

bool AdressResolverPass::runOnMachineFunction(MachineFunction &F) {
  setupEncoder(F);

  // Parse the linked binary once, lazily: the disassembler needs the subtarget,
  // which is only available from a machine function.
  if (!Parsed) {
    Parsed = true;
    if (!ElfFilename.empty()) {
      parseElf(F);
    } else {
      // No linked binary: addresses, the memory model and library-call costs
      // are all unavailable. Run MIR-only and flag the result as unsound.
      TAR.addUnsoundReason(
          "no -elf-file: no linked-binary grounding (memory model and "
          "library-call costs omitted)");
    }
    finishParse();
  }

  if (!AnalyzedComputed) {
    computeAnalyzedSet(getAnalysis<CallGraphWrapperPass>().getCallGraph());
    AnalyzedComputed = true;
  }
  const bool Diagnose = isAnalyzed(F);

  auto It = FunctionEntryAddr.find(F.getName().str());
  if (It == FunctionEntryAddr.end()) {
    if (Diagnose)
      ++Cov.FunctionsNoDumpEntry;
    if (AddressResolverVerbose && Diagnose)
      outs() << "[addr-resolver] no dump entry for analysed function '"
             << F.getName() << "', skipping address resolution\n";
    return false;
  }

  uint64_t Entry = It->second;
  // End of this function = first function entry strictly greater than Entry.
  uint64_t End = std::numeric_limits<uint64_t>::max();
  auto Hi =
      std::upper_bound(SortedEntryAddrs.begin(), SortedEntryAddrs.end(), Entry);
  if (Hi != SortedEntryAddrs.end())
    End = *Hi;

  // Collect the dump instructions belonging to this function, in address order.
  std::vector<const DumpInstruction *> Range;
  for (const auto &DI : DumpInstructions)
    if (DI.Address >= Entry && DI.Address < End)
      Range.push_back(&DI);
  std::sort(Range.begin(), Range.end(),
            [](const DumpInstruction *A, const DumpInstruction *B) {
              return A->Address < B->Address;
            });

  if (Diagnose)
    ++Cov.Functions;
  alignFunction(F, Range, Diagnose);
  return false;
}

//===----------------------------------------------------------------------===//
// Section classification
//===----------------------------------------------------------------------===//

/// Classifies an ELF section by name. Code = executable sections; Data =
/// everything that holds objects (.data/.bss/.rodata/.heap/...); Ignore = debug
/// and metadata. Classification is by exclusion so new data section names
/// (e.g. .rodata2, .fram_smallheap) are picked up automatically.
AdressResolverPass::SectionClass
AdressResolverPass::classifySection(StringRef Name) {
  if (Name == ".text" || Name.starts_with("__reset_vector") ||
      Name.starts_with("__interrupt_vector"))
    return SectionClass::Code;
  if (Name.starts_with(".debug") || Name == ".comment" ||
      Name == ".MSP430.attributes" || Name == ".symtab" || Name == ".strtab" ||
      Name == ".shstrtab")
    return SectionClass::Ignore;
  return SectionClass::Data;
}

//===----------------------------------------------------------------------===//
// ELF parsing
//===----------------------------------------------------------------------===//

void AdressResolverPass::parseElf(const MachineFunction &F) {
  using namespace llvm::object;

  // Build a disassembler from the active subtarget. DisCtx must outlive DisAsm,
  // so declare it first (locals destruct in reverse order).
  const TargetMachine &TM = F.getTarget();
  const MCSubtargetInfo *Sti = &F.getSubtarget();
  const MCAsmInfo *MAI = TM.getMCAsmInfo();
  const MCRegisterInfo *MRI = TM.getMCRegisterInfo();
  if (!MAI || !MRI || !Sti) {
    errs() << "[addr-resolver] warning: missing MC info; cannot disassemble "
              "the ELF\n";
    return;
  }
  MCContext DisCtx(TM.getTargetTriple(), MAI, MRI, Sti);
  std::unique_ptr<MCDisassembler> DisAsm(
      TM.getTarget().createMCDisassembler(*Sti, DisCtx));
  if (!DisAsm) {
    errs() << "[addr-resolver] warning: no disassembler for this target; ELF "
              "address resolution disabled\n";
    return;
  }

  // Open the linked ELF.
  ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr =
      MemoryBuffer::getFile(ElfFilename);
  if (!BufOrErr) {
    errs() << "[addr-resolver] warning: could not open -elf-file '"
           << ElfFilename << "': " << BufOrErr.getError().message() << "\n";
    return;
  }
  Expected<std::unique_ptr<ObjectFile>> ObjOrErr =
      ObjectFile::createObjectFile((*BufOrErr)->getMemBufferRef());
  if (!ObjOrErr) {
    logAllUnhandledErrors(ObjOrErr.takeError(), errs(),
                          "[addr-resolver] could not parse -elf-file: ");
    return;
  }
  ObjectFile &Obj = **ObjOrErr;

  // 1) Symbols -> function entries, data objects, and the union of symbol
  // addresses (for data-object size derivation).
  for (const SymbolRef &Sym : Obj.symbols()) {
    Expected<uint32_t> FlagsOrErr = Sym.getFlags();
    if (!FlagsOrErr) {
      consumeError(FlagsOrErr.takeError());
      continue;
    }
    if (*FlagsOrErr & SymbolRef::SF_Undefined)
      continue;

    Expected<StringRef> NameOrErr = Sym.getName();
    Expected<uint64_t> AddrOrErr = Sym.getAddress();
    if (!NameOrErr || !AddrOrErr) {
      if (!NameOrErr)
        consumeError(NameOrErr.takeError());
      if (!AddrOrErr)
        consumeError(AddrOrErr.takeError());
      continue;
    }
    StringRef Name = *NameOrErr;
    uint64_t Addr = *AddrOrErr;
    if (Name.empty())
      continue;

    // Resolve the symbol's section name for classification.
    StringRef SecName;
    if (Expected<section_iterator> SecOrErr = Sym.getSection()) {
      section_iterator SecIt = *SecOrErr;
      if (SecIt != Obj.section_end()) {
        if (Expected<StringRef> N = SecIt->getName())
          SecName = *N;
        else
          consumeError(N.takeError());
      }
    } else {
      consumeError(SecOrErr.takeError());
    }

    Expected<SymbolRef::Type> TyOrErr = Sym.getType();
    SymbolRef::Type Ty = TyOrErr ? *TyOrErr : SymbolRef::ST_Unknown;
    if (!TyOrErr)
      consumeError(TyOrErr.takeError());

    AllSymbolAddrs.push_back(Addr);

    if (Ty == SymbolRef::ST_Function) {
      // Real function (STT_FUNC) -> entry address. Mirrors the dump path, which
      // only recorded "<name>():" symbols. Keep the first if a name repeats.
      FunctionEntryAddr.try_emplace(Name.str(), Addr);
    } else if (classifySection(SecName) == SectionClass::Data) {
      TimingAnalysisResults::DataObject DObj;
      DObj.Name = Name.str();
      DObj.Address = Addr;
      DObj.Section = SecName.str();
      DataObjects.push_back(std::move(DObj));
    }
  }

  // 2) Code sections -> per-instruction {address, bytes} via the disassembler.
  // alignFunction consumes these exactly as it did the dump entries.
  for (const SectionRef &Sec : Obj.sections()) {
    StringRef SecName;
    if (Expected<StringRef> N = Sec.getName())
      SecName = *N;
    else
      consumeError(N.takeError());
    if (classifySection(SecName) != SectionClass::Code)
      continue;

    Expected<StringRef> ContentsOrErr = Sec.getContents();
    if (!ContentsOrErr) {
      consumeError(ContentsOrErr.takeError());
      continue;
    }
    StringRef Contents = *ContentsOrErr;
    const uint64_t Base = Sec.getAddress();
    ArrayRef<uint8_t> Data(reinterpret_cast<const uint8_t *>(Contents.data()),
                           Contents.size());

    uint64_t Off = 0;
    while (Off < Data.size()) {
      MCInst Inst;
      uint64_t InstSize = 0;
      MCDisassembler::DecodeStatus St = DisAsm->getInstruction(
          Inst, InstSize, Data.slice(Off), Base + Off, nulls());
      if (St != MCDisassembler::Success || InstSize == 0) {
        // Data/padding inside a code section: step the minimum MSP430
        // instruction width and resync.
        Off += 2;
        continue;
      }
      DumpInstruction DI;
      DI.Address = Base + Off;
      DI.Bytes.assign(Data.begin() + Off, Data.begin() + Off + InstSize);
      DI.MachineCode = toHex(DI.Bytes);
      DumpInstructions.push_back(std::move(DI));
      Off += InstSize;
    }
  }

  if (AddressResolverVerbose)
    outs() << "[addr-resolver] decoded ELF '" << ElfFilename << "'\n";
}

//===----------------------------------------------------------------------===//
// Encoding cross-check
//===----------------------------------------------------------------------===//

void AdressResolverPass::setupEncoder(MachineFunction &F) {
  if (EncoderReady)
    return;
  EncoderReady = true;

  const TargetMachine &TM = F.getTarget();
  STI = &F.getSubtarget();
  const MCAsmInfo *MAI = TM.getMCAsmInfo();
  const MCRegisterInfo *MRI = TM.getMCRegisterInfo();
  const MCInstrInfo *MII = TM.getMCInstrInfo();
  if (!MAI || !MRI || !MII || !STI) {
    EncoderUsable = false;
    return;
  }

  Ctx = std::make_unique<MCContext>(TM.getTargetTriple(), MAI, MRI, STI);
  CodeEmitter.reset(TM.getTarget().createMCCodeEmitter(*MII, *Ctx));
  EncoderUsable = (CodeEmitter != nullptr);
  if (!EncoderUsable && AddressResolverVerbose)
    outs() << "[addr-resolver] no MC code emitter available; encoding "
              "cross-check disabled\n";
}

bool AdressResolverPass::tryEncode(const MachineInstr &MI,
                                   std::vector<uint8_t> &Bytes) {
  if (!EncoderUsable)
    return false;

  // Pseudo instructions have no machine encoding; the MC code emitter
  // report_fatal_error()s on them (it cannot be caught). They are never useful
  // re-sync anchors, so treat them as non-anchorable up front.
  if (MI.isPseudo())
    return false;

  // Only "simple" instructions (register/immediate/regmask operands) are
  // encodable here; anything with a symbol/branch/address operand would need a
  // live AsmPrinter and would carry a fixup we cannot compare to linked bytes.
  MCInst Inst;
  Inst.setOpcode(MI.getOpcode());
  for (const MachineOperand &MO : MI.operands()) {
    switch (MO.getType()) {
    case MachineOperand::MO_Register:
      if (MO.isImplicit())
        continue;
      Inst.addOperand(MCOperand::createReg(MO.getReg()));
      break;
    case MachineOperand::MO_Immediate:
      Inst.addOperand(MCOperand::createImm(MO.getImm()));
      break;
    case MachineOperand::MO_RegisterMask:
      continue;
    default:
      return false; // not anchorable
    }
  }

  SmallVector<char, 8> CB;
  SmallVector<MCFixup, 2> Fixups;
  CodeEmitter->encodeInstruction(Inst, CB, Fixups, *STI);
  if (!Fixups.empty())
    return false; // address operand -> bytes are a relocation placeholder

  Bytes.assign(CB.begin(), CB.end());
  return true;
}

//===----------------------------------------------------------------------===//
// Alignment
//===----------------------------------------------------------------------===//

static bool isCodeEmitting(const MachineInstr &MI) {
  // Mirrors the zero-latency cases in getMSP430Latency: debug values and CFI
  // pseudo-instructions emit no machine code and have no dump entry.
  return !MI.isDebugInstr() && !MI.isCFIInstruction();
}

static std::string toHex(const std::vector<uint8_t> &Bytes) {
  std::string S;
  for (size_t I = 0; I < Bytes.size(); ++I) {
    if (I)
      S.push_back(' ');
    char Buf[3];
    std::snprintf(Buf, sizeof(Buf), "%02x", Bytes[I]);
    S += Buf;
  }
  return S;
}

void AdressResolverPass::alignFunction(
    MachineFunction &F, std::vector<const DumpInstruction *> &Range,
    bool Diagnose) {
  // Address resolution always runs; diagnostics are emitted only for functions
  // LLTA actually analyses, so warnings stay relevant.
  const bool V = AddressResolverVerbose && Diagnose;
  const StringRef FName = F.getName();
  size_t DumpIdx = 0;
  unsigned Warnings = 0;
  unsigned MiIdx = 0;

  for (MachineBasicBlock &MBB : F) {
    for (MachineInstr &MI : MBB) {
      if (!isCodeEmitting(MI))
        continue;
      ++MiIdx;
      if (Diagnose)
        ++Cov.CodeMIs;

      if (DumpIdx >= Range.size()) {
        ++Warnings;
        if (V) {
          errs() << "[addr-resolver] " << FName
                 << ": ran out of dump entries "
                    "(MI #"
                 << MiIdx << "): ";
          MI.print(errs());
        }
        continue;
      }

      std::vector<uint8_t> Bytes;
      bool Anchor = tryEncode(MI, Bytes);

      // A length difference means the MC encoder chose a different (but
      // equivalent) addressing form than the linker/objdump did -- e.g. an
      // indexed "0(Rn)" word vs. an indirect "@Rn" without it. That is not a
      // real misalignment, so do not treat such an instruction as an anchor.
      bool UsableAnchor =
          Anchor && Bytes.size() == Range[DumpIdx]->Bytes.size();
      if (Anchor && !UsableAnchor && V)
        outs() << "[addr-resolver] " << FName << ": MI #" << MiIdx
               << " encodes to a different addressing form than the dump "
                  "(enc ["
               << toHex(Bytes) << "] vs dump [" << Range[DumpIdx]->MachineCode
               << "]); skipping as anchor\n";

      if (UsableAnchor && Range[DumpIdx]->Bytes != Bytes) {
        // Same length but different bytes: the cursor likely drifted (e.g. an
        // inline-asm body consumed extra dump entries). Search forward for the
        // next position whose bytes match this anchor.
        size_t K = DumpIdx + 1;
        size_t Lim = std::min(Range.size(), DumpIdx + 1 + ResyncWindow);
        while (K < Lim && Range[K]->Bytes != Bytes)
          ++K;
        if (K < Lim) {
          ++Warnings;
          if (Diagnose)
            ++Cov.ResyncEvents;
          if (V)
            errs() << "[addr-resolver] " << FName << ": re-synced at MI #"
                   << MiIdx << ", skipped " << (K - DumpIdx)
                   << " unattributed dump word-group(s) [0x"
                   << Twine::utohexstr(Range[DumpIdx]->Address) << "..0x"
                   << Twine::utohexstr(Range[K]->Address)
                   << ") (likely inline asm / unhandled expansion)\n";
          DumpIdx = K;
        } else {
          ++Warnings;
          if (Diagnose)
            ++Cov.MismatchEvents;
          if (V) {
            errs() << "[addr-resolver] " << FName
                   << ": encoding mismatch at MI #" << MiIdx << " (dump 0x"
                   << Twine::utohexstr(Range[DumpIdx]->Address)
                   << "): expected [" << toHex(Bytes) << "] got ["
                   << Range[DumpIdx]->MachineCode << "] asm='"
                   << Range[DumpIdx]->AssemblerCode << "' MI=";
            MI.print(errs());
          }
          // Best-effort: assign positionally so coverage stays maximal.
        }
      }

      TAR.setInstructionAddress(&MI, Range[DumpIdx]->Address);
      if (Diagnose)
        ++Cov.ResolvedMIs;
      if (Range[DumpIdx]->HasTarget) {
        TAR.setBranchTarget(&MI, Range[DumpIdx]->TargetAddress);
        if (Diagnose)
          ++Cov.BranchTargets;
      }
      if (V) {
        outs() << "[addr-resolver]   0x"
               << Twine::utohexstr(Range[DumpIdx]->Address) << "  "
               << Range[DumpIdx]->AssemblerCode;
        if (Range[DumpIdx]->HasTarget)
          outs() << " -> 0x" << Twine::utohexstr(Range[DumpIdx]->TargetAddress);
        outs() << "\n";
      }
      ++DumpIdx;
    }
  }

  if (DumpIdx != Range.size()) {
    ++Warnings;
    if (Diagnose)
      ++Cov.LeftoverEvents;
    if (V)
      errs() << "[addr-resolver] " << FName << ": " << (Range.size() - DumpIdx)
             << " dump entr(y/ies) left unassigned at end of function "
                "(possible misalignment)\n";
  }
  if (Warnings && Diagnose && !AddressResolverVerbose)
    errs() << "[addr-resolver] " << FName << ": " << Warnings
           << " address cross-check warning(s); rerun with "
              "-address-resolver-verbose for details\n";
}

MachineFunctionPass *createAdressResolverPass(TimingAnalysisResults &TAR) {
  return new AdressResolverPass(TAR);
}
} // namespace llvm
