#ifndef LLTA_TARGETS_MSP430_MSP430TARGET_H
#define LLTA_TARGETS_MSP430_MSP430TARGET_H

#include "Targets/MSP430/MSP430Pipeline.h"
#include "Targets/RTTarget.h"

namespace llvm {
class MachineInstr;
} // namespace llvm

namespace llta {

/// Single source of truth for MSP430 base instruction latencies (zero memory
/// wait states). Used both by MSP430Target::getInstructionLatency and by the
/// MSP430 pipeline's execution stage so the two never diverge.
unsigned getMSP430Latency(const llvm::MachineInstr &MI);

/// MSP430 family base. Holds the ISA-level timing behavior shared by every
/// MSP430 device: the instruction latency table, instruction validation, and
/// the 16-bit fetch width. Device specifics (memory model, clock, caches) are
/// added by device subclasses such as MSP430FR5994Target.
class MSP430Target : public RTTarget {
public:
  llvm::StringRef getName() const override { return "MSP430"; }
  llvm::Triple::ArchType getArch() const override {
    return llvm::Triple::msp430;
  }

  unsigned getInstructionLatency(const llvm::MachineInstr &MI) const override;
  void checkInstruction(const llvm::MachineInstr &MI) const override;
  unsigned getMaxInstructionWords() const override { return 3; }

  bool isControlFlowMnemonic(llvm::StringRef Mnemonic) const override;
  std::optional<uint64_t>
  resolveBranchTarget(llvm::StringRef Mnemonic,
                      llvm::StringRef Comment) const override;

  /// MSP430 uses a simple 1-stage pipeline (see MSP430Pipeline). Owned here and
  /// handed to the WorklistSolver; mutable because the solver drives it.
  llvm::AbstractAnalysable &getPipeline() const override { return Pipeline; }

private:
  mutable llvm::MSP430Pipeline Pipeline;
};

} // namespace llta

#endif // LLTA_TARGETS_MSP430_MSP430TARGET_H
