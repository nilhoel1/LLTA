#ifndef LLTA_ADRESS_RESOLVER_H
#define LLTA_ADRESS_RESOLVER_H
#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace llta {
class RTTarget;
} // namespace llta

namespace llvm {

class MCContext;
class MCCodeEmitter;
class MCSubtargetInfo;
class MachineInstr;
class CallGraph;
class Function;

class AdressResolverPass : public MachineFunctionPass {
public:
  static char ID;

  TimingAnalysisResults &TAR;
  AdressResolverPass(TimingAnalysisResults &TAR);

  bool runOnMachineFunction(MachineFunction &F) override;
  bool doFinalization(Module &) override;
  bool doInitialization(Module &) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  virtual llvm::StringRef getPassName() const override {
    return "Adress Resolver Pass";
  }

  /// A single instruction decoded from the linked ELF's code sections by the
  /// MCDisassembler. The disassembler reports the full instruction length, so
  /// extension words are already part of Bytes (no continuation merging
  /// needed).
  struct DumpInstruction {
    uint64_t Address = 0;
    std::vector<uint8_t> Bytes;
    std::string MachineCode;    ///< hex of Bytes, for diagnostics
    std::string AssemblerCode;  ///< mnemonic + operands, for diagnostics
    uint64_t TargetAddress = 0; ///< static branch/call target, if any
    bool HasTarget = false;
  };

  /// Classification of an ELF section, to decide how its symbols are treated.
  enum class SectionClass { Code, Data, Ignore, Unknown };

private:
  /// Parsed dump instructions, in file order.
  std::vector<DumpInstruction> DumpInstructions;
  /// Real function symbol name -> entry address.
  std::map<std::string, uint64_t> FunctionEntryAddr;
  /// Sorted unique function entry addresses, to compute function end bounds.
  std::vector<uint64_t> SortedEntryAddrs;

  /// Data/heap objects discovered in data sections, staged before being pushed
  /// into TimingAnalysisResults (sizes are filled in once parsing completes).
  std::vector<TimingAnalysisResults::DataObject> DataObjects;
  /// Address of every "<name>:" header (code and data), to derive object sizes
  /// as "next symbol address minus this address".
  std::vector<uint64_t> AllSymbolAddrs;

  /// Set of functions LLTA actually analyses (call-graph reachable from the
  /// start function). Address resolution still runs for every function, but
  /// cross-check diagnostics are emitted only for these. Computed lazily.
  bool AnalyzedComputed = false;
  bool AnalyzeAll = false; ///< true when no single start function is determined
  std::set<const Function *> AnalyzedFunctions;

  /// Lazily built MC machinery used for the encoding cross-check.
  bool EncoderReady = false;
  bool EncoderUsable = false;
  std::unique_ptr<MCContext> Ctx;
  std::unique_ptr<MCCodeEmitter> CodeEmitter;
  const MCSubtargetInfo *STI = nullptr;

  // --- ELF parsing (preferred; drives address resolution + ABI costing) ---
  /// Decode the linked ELF (Utility/Options ElfFilename) into DumpInstructions,
  /// FunctionEntryAddr and DataObjects, using llvm::object::ObjectFile +
  /// MCDisassembler. Needs a MachineFunction for the active subtarget. Runs
  /// once (guarded by Parsed). A no-op (leaving the maps empty) if no ELF is
  /// supplied.
  void parseElf(const MachineFunction &F);
  /// True once parsing has been attempted (ELF or legacy dump).
  bool Parsed = false;
  /// Post-process parsed symbols: sort unique entry addresses and derive each
  /// data object's size.
  void finishParse();

  /// Classify a section name (e.g. ".data") as code, data, or ignorable.
  static SectionClass classifySection(StringRef Name);

  // --- encoding cross-check ---
  void setupEncoder(MachineFunction &F);
  /// Returns true and fills \p Bytes if \p MI is "anchorable": all operands are
  /// register/immediate/regmask and the encoding emitted no fixups (so its
  /// bytes equal the linked dump bytes exactly).
  bool tryEncode(const MachineInstr &MI, std::vector<uint8_t> &Bytes);

  // --- analysed-subgraph scoping (for diagnostics only) ---
  void computeAnalyzedSet(CallGraph &CG);
  Function *getStartingFunction(CallGraph &CG);
  bool isAnalyzed(const MachineFunction &F) const;

  // --- alignment ---
  void alignFunction(MachineFunction &F,
                     std::vector<const DumpInstruction *> &Range,
                     bool Diagnose);

  // --- coverage statistics (verify-only; printed under
  // -address-resolver-verbose) --- Accumulated only over the functions LLTA
  // actually analyses, so the numbers reflect what the timing analysis depends
  // on, not the whole linked ELF.
  struct Coverage {
    unsigned Functions = 0; ///< analysed functions with a dump entry
    unsigned FunctionsNoDumpEntry =
        0;                       ///< analysed functions missing a dump entry
    uint64_t CodeMIs = 0;        ///< code-emitting MIs seen
    uint64_t ResolvedMIs = 0;    ///< MIs that received an address
    uint64_t BranchTargets = 0;  ///< control-flow MIs that received a target
    unsigned ResyncEvents = 0;   ///< forward re-syncs (likely inline asm)
    unsigned MismatchEvents = 0; ///< byte mismatches with no re-sync found
    unsigned LeftoverEvents = 0; ///< functions with dump entries left over
  } Cov;
};
} // namespace llvm

namespace llvm {
MachineFunctionPass *createAdressResolverPass(TimingAnalysisResults &TAR);
} // namespace llvm
#endif
