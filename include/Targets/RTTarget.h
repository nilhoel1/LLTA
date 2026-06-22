#ifndef LLTA_TARGETS_RTTARGET_H
#define LLTA_TARGETS_RTTARGET_H

#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace llvm {
class MachineInstr;
class MCInst;
class MachineFunctionPass;
class TimingAnalysisResults;
class AbstractAnalysable;
} // namespace llvm

namespace llta {

/// Abstract description of a real-time target (a concrete device) for timing
/// analysis. A target is resolved from the LLVM Triple by the target registry
/// (see TargetRegistry.h) in two stages: the LLVM arch selects a *family* base,
/// then a device-resolution step selects the concrete *device* subclass.
///
/// ISA-level behavior (instruction latencies, instruction validation, fetch
/// width) lives on the family base; device-specific behavior (memory model,
/// clock, cache geometry, target-specific options) lives on the device
/// subclass. Generic analysis passes query the active target through this
/// interface instead of switching on the arch, so adding a target is a new
/// subclass plus a registry entry.
class RTTarget {
public:
  virtual ~RTTarget() = default;

  //===--- Identity -------------------------------------------------------===//

  /// Human-readable device name, e.g. "MSP430FR5994".
  virtual llvm::StringRef getName() const = 0;

  /// The LLVM arch this target's family belongs to.
  virtual llvm::Triple::ArchType getArch() const = 0;

  //===--- ISA-level (family) ---------------------------------------------===//

  /// Base latency of \p MI in CPU cycles, assuming zero memory wait states.
  /// Device-specific memory penalties (e.g. FRAM wait states) are added by the
  /// target's memory-model passes, not here.
  virtual unsigned
  getInstructionLatency(const llvm::MachineInstr &MI) const = 0;

  /// Base latency of an instruction decoded from the linked ELF (an `MCInst`),
  /// or nullopt if this target has no latency model for it. Used to cost
  /// library-call (ABI) routines whose bodies are absent from the analyzed IR.
  /// Default: none (no ABI costing) — the caller then reports the result as
  /// unsound.
  virtual std::optional<unsigned>
  getInstructionLatency(const llvm::MCInst &MI) const {
    return std::nullopt;
  }

  /// Validate that \p MI conforms to the timing model's assumptions (e.g. no
  /// un-lowered pseudo instructions). Diagnoses on errs() and asserts on a
  /// violation; a no-op for instructions the model understands.
  virtual void checkInstruction(const llvm::MachineInstr &MI) const = 0;

  /// Worst-case cycle cost to charge for a call into the external function
  /// \p CalleeName whose body is absent from the analyzed IR (e.g. a libgcc
  /// `__mspabi_*` soft-float routine, or libm `sin`/`cos`). This is the callee
  /// *body* cost only — the `call` instruction itself is costed at the call
  /// site. Returns nullopt if the target has no cost for that callee, in which
  /// case the call is left uncosted and the WCET is reported as unsound.
  /// Default: none (no library-call costing).
  virtual std::optional<unsigned>
  getExternalCallCost(llvm::StringRef CalleeName) const {
    return std::nullopt;
  }

  /// Maximum number of 16-bit code words a single instruction can fetch. Used
  /// to cap the per-instruction fetch-word count derived from address gaps so a
  /// layout gap cannot inflate it.
  virtual unsigned getMaxInstructionWords() const = 0;

  //===--- Disassembly parsing (objdump dump) -----------------------------===//

  /// True if \p Mnemonic is a jump/call/branch that can carry a static target
  /// in its objdump trailing comment.
  virtual bool isControlFlowMnemonic(llvm::StringRef Mnemonic) const = 0;

  /// Parse the static branch/call target of a control-flow instruction out of
  /// its objdump trailing comment \p Comment (the text after ';'). Returns the
  /// target address, or std::nullopt if \p Mnemonic is not control-flow or no
  /// static target is present (e.g. indirect transfers).
  virtual std::optional<uint64_t>
  resolveBranchTarget(llvm::StringRef Mnemonic,
                      llvm::StringRef Comment) const = 0;

  //===--- Memory model ---------------------------------------------------===//

  /// Machine-function passes this target contributes to model its memory
  /// subsystem (e.g. MSP430FR's FRAM wait-state / read-cache passes). They are
  /// spliced into the pipeline right after InstructionLatencyPass so their
  /// penalties land in MBBLatencyMap. Default: none (no extra memory model).
  /// Returned passes are owned by the caller (the pass pipeline).
  virtual std::vector<llvm::MachineFunctionPass *>
  getMemoryModelPasses(llvm::TimingAnalysisResults &TAR) const {
    return {};
  }

  //===--- Microarchitecture ----------------------------------------------===//

  /// The target's microarchitectural pipeline model, used as the transfer
  /// function for the abstract-interpretation WorklistSolver in
  /// PathAnalysisPass. Owned by the target (which outlives the analysis);
  /// returned by mutable reference because the solver drives it.
  virtual llvm::AbstractAnalysable &getPipeline() const = 0;
};

} // namespace llta

#endif // LLTA_TARGETS_RTTARGET_H
