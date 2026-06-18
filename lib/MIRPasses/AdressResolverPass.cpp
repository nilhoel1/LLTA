#include "MIRPasses/AdressResolverPass.h"
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
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <fstream>
#include <limits>

namespace llvm {

char AdressResolverPass::ID = 1;

/// How far forward to search for a re-sync point when an anchor instruction
/// does not byte-match at the current dump cursor (e.g. an inline-asm body or
/// an unhandled expansion sits in between).
static constexpr size_t ResyncWindow = 32;

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
  parseFile(DumpFilename);

  // Collect sorted, unique function entry addresses so we can derive the end of
  // each function as "the next function entry".
  SortedEntryAddrs.clear();
  for (const auto &KV : FunctionEntryAddr)
    SortedEntryAddrs.push_back(KV.second);
  std::sort(SortedEntryAddrs.begin(), SortedEntryAddrs.end());
  SortedEntryAddrs.erase(
      std::unique(SortedEntryAddrs.begin(), SortedEntryAddrs.end()),
      SortedEntryAddrs.end());

  // Parse and store the FRAM start address (foundation only).
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

  // Derive each data object's size as "next symbol address minus its own
  // address", using the sorted set of all symbol header addresses (code+data).
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
    size_t Targets = 0;
    for (const auto &DI : DumpInstructions)
      if (DI.HasTarget)
        ++Targets;
    outs() << "[addr-resolver] parsed " << DumpInstructions.size()
           << " dump instructions, " << FunctionEntryAddr.size()
           << " function entries, " << DataObjects.size()
           << " data objects, " << Targets << " static jump/call targets from "
           << DumpFilename << "\n";
    for (const auto &Obj : DataObjects)
      outs() << "[addr-resolver]   data " << Obj.Name << " @0x"
             << Twine::utohexstr(Obj.Address) << " size " << Obj.Size << " ("
             << Obj.Section << ")\n";
  }
  return false;
}

bool AdressResolverPass::runOnMachineFunction(MachineFunction &F) {
  setupEncoder(F);

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
  auto Hi = std::upper_bound(SortedEntryAddrs.begin(), SortedEntryAddrs.end(),
                             Entry);
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
// Dump parsing
//===----------------------------------------------------------------------===//

bool AdressResolverPass::isHexStr(StringRef S) {
  if (S.empty())
    return false;
  for (char C : S)
    if (!isxdigit(static_cast<unsigned char>(C)))
      return false;
  return true;
}

/// Classifies an objdump section by name. Code = executable sections; Data =
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

bool AdressResolverPass::isControlFlowMnemonic(StringRef Mnemonic) {
  // MSP430 jumps (real + emulated), plus call and br.
  static const char *const CF[] = {"jmp", "jeq", "jz",  "jne", "jnz", "jc",
                                   "jhs", "jnc", "jlo", "jn",  "jge", "jl",
                                   "call", "br"};
  for (const char *M : CF)
    if (Mnemonic == M)
      return true;
  return false;
}

void AdressResolverPass::resolveTarget(StringRef Mnemonic, StringRef Comment,
                                       DumpInstruction &Out) {
  if (!isControlFlowMnemonic(Mnemonic))
    return;
  StringRef C = Comment.trim();
  // Jumps print "abs 0x4056"; calls/br print "#0x402c".
  if (C.consume_front("abs"))
    C = C.trim();
  else if (C.consume_front("#"))
    C = C.trim();
  else
    return;
  uint64_t Target = 0;
  if (C.getAsInteger(0, Target)) // 0 => honour the "0x" prefix
    return;
  Out.TargetAddress = Target;
  Out.HasTarget = true;
}

/// Parses a symbol header line of the form "0000401c <main>:" into its address
/// and symbol name. Returns false for anything else.
bool AdressResolverPass::parseHeaderLine(const std::string &Line, uint64_t &Addr,
                                         std::string &Name) {
  std::string::size_type Lt = Line.find('<');
  std::string::size_type Gt = Line.find(">:");
  if (Lt == std::string::npos || Gt == std::string::npos || Gt < Lt)
    return false;
  StringRef Left = StringRef(Line).substr(0, Lt).trim();
  if (!isHexStr(Left) || Left.getAsInteger(16, Addr))
    return false;
  Name = Line.substr(Lt + 1, Gt - (Lt + 1));
  return true;
}

/// Parses an instruction line of the form
///   "    401c:\tb0 12 2c 40 \tcall\t#16428\t;#0x402c"
/// Returns false if the line is not an instruction (no leading "hexaddr:" with
/// at least one machine-code byte). \p HasAsm is set to false for continuation
/// lines (extension words printed on their own line, no mnemonic).
bool AdressResolverPass::parseInstructionLine(const std::string &Line,
                                              DumpInstruction &Out,
                                              bool &HasAsm) {
  std::string::size_type Colon = Line.find(':');
  if (Colon == std::string::npos || Colon == 0)
    return false;
  StringRef Left = StringRef(Line).substr(0, Colon).trim();
  uint64_t Addr = 0;
  if (!isHexStr(Left) || Left.getAsInteger(16, Addr))
    return false;

  // Split off the trailing comment; it carries the static branch/call target.
  std::string Rest = Line.substr(Colon + 1);
  std::string Comment;
  std::string::size_type Semi = Rest.find(';');
  if (Semi != std::string::npos) {
    Comment = Rest.substr(Semi + 1);
    Rest = Rest.substr(0, Semi);
  }

  // Tokenise on whitespace (space and tab).
  std::vector<std::string> Tokens;
  {
    std::string Cur;
    for (char C : Rest) {
      if (C == ' ' || C == '\t' || C == '\r') {
        if (!Cur.empty()) {
          Tokens.push_back(Cur);
          Cur.clear();
        }
      } else {
        Cur.push_back(C);
      }
    }
    if (!Cur.empty())
      Tokens.push_back(Cur);
  }

  // Leading two-hex-digit tokens are machine-code bytes.
  std::vector<uint8_t> Bytes;
  size_t I = 0;
  for (; I < Tokens.size(); ++I) {
    if (Tokens[I].size() == 2 && isHexStr(Tokens[I]))
      Bytes.push_back(static_cast<uint8_t>(std::stoul(Tokens[I], nullptr, 16)));
    else
      break;
  }
  if (Bytes.empty())
    return false; // not an instruction line

  std::string Asm;
  for (; I < Tokens.size(); ++I) {
    if (!Asm.empty())
      Asm.push_back(' ');
    Asm += Tokens[I];
  }

  std::string MachineCode;
  for (size_t B = 0; B < Bytes.size(); ++B) {
    if (B)
      MachineCode.push_back(' ');
    char Buf[3];
    std::snprintf(Buf, sizeof(Buf), "%02x", Bytes[B]);
    MachineCode += Buf;
  }

  Out.Address = Addr;
  Out.Bytes = std::move(Bytes);
  Out.MachineCode = std::move(MachineCode);
  Out.AssemblerCode = Asm;
  HasAsm = !Asm.empty();

  // The first asm token is the mnemonic; resolve a static target from the
  // comment for control-flow instructions.
  StringRef Mnemonic = StringRef(Out.AssemblerCode).split(' ').first;
  resolveTarget(Mnemonic, Comment, Out);
  return true;
}

void AdressResolverPass::parseFile(StringRef FilePath) {
  std::ifstream In(FilePath.str().c_str());
  if (!In.is_open()) {
    errs() << "[addr-resolver] warning: could not open dump file '" << FilePath
           << "'\n";
    return;
  }

  std::string Line;
  bool PendingHeader = false;
  uint64_t PendingAddr = 0;
  std::string PendingName;
  SectionClass CurClass = SectionClass::Unknown;
  std::string CurSection;

  while (std::getline(In, Line)) {
    // Section marker, e.g. "Disassembly of section .data:". Switches the
    // classification used for subsequent symbols.
    StringRef T = StringRef(Line).trim();
    if (T.consume_front("Disassembly of section ") && T.ends_with(":")) {
      CurSection = T.drop_back(1).str(); // strip trailing ':'
      CurClass = classifySection(CurSection);
      PendingHeader = false;
      continue;
    }

    // Resolve a pending header: the first non-empty line after a "<NAME>:"
    // header tells us whether NAME is a real function ("NAME():") or just an
    // internal label (a source path or an instruction).
    if (PendingHeader) {
      StringRef Trimmed = StringRef(Line).trim();
      if (Trimmed.empty())
        continue; // keep the header pending across blank lines
      // Only record real functions, and only in code sections, so data symbols
      // that happen to print a "NAME():" line (e.g. __heap_start__) are not
      // mistaken for functions.
      if (Trimmed.ends_with("():") && CurClass != SectionClass::Data &&
          CurClass != SectionClass::Ignore)
        FunctionEntryAddr[PendingName] = PendingAddr;
      PendingHeader = false;
      // Fall through: this line is never itself an instruction line.
    }

    uint64_t HdrAddr = 0;
    std::string HdrName;
    if (parseHeaderLine(Line, HdrAddr, HdrName)) {
      AllSymbolAddrs.push_back(HdrAddr);
      if (CurClass == SectionClass::Data) {
        // Data/heap object: record it directly (size is derived once the whole
        // file is parsed); no function "():" check needed.
        TimingAnalysisResults::DataObject Obj;
        Obj.Name = HdrName;
        Obj.Address = HdrAddr;
        Obj.Section = CurSection;
        DataObjects.push_back(std::move(Obj));
        continue;
      }
      PendingHeader = true;
      PendingAddr = HdrAddr;
      PendingName = HdrName;
      continue;
    }

    DumpInstruction DI;
    bool HasAsm = false;
    if (parseInstructionLine(Line, DI, HasAsm)) {
      if (!HasAsm && !DumpInstructions.empty()) {
        // Continuation line: append its extension words to the previous
        // instruction instead of creating a new entry.
        DumpInstruction &Prev = DumpInstructions.back();
        Prev.Bytes.insert(Prev.Bytes.end(), DI.Bytes.begin(), DI.Bytes.end());
        if (!DI.MachineCode.empty())
          Prev.MachineCode += " " + DI.MachineCode;
      } else {
        DumpInstructions.push_back(std::move(DI));
      }
    }
  }
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
          errs() << "[addr-resolver] " << FName << ": ran out of dump entries "
                    "(MI #" << MiIdx << "): ";
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
                  "(enc [" << toHex(Bytes) << "] vs dump ["
               << Range[DumpIdx]->MachineCode
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
            errs() << "[addr-resolver] " << FName << ": encoding mismatch at MI #"
                   << MiIdx << " (dump 0x"
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
