#ifndef LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H
#define LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H

#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include <cstddef>
#include <unordered_map>

namespace llvm {

/**
 * This pass sums up the base instruction latency of each basic block in a
 * function into MBBLatencyMap. The per-instruction latency comes from the
 * active target (RTTarget::getInstructionLatency), so the pass itself is
 * target-agnostic. Memory penalties (e.g. FRAM wait states) are added later by
 * the target's contributed memory-model passes.
 */
class InstructionLatencyPass : public MachineFunctionPass {
public:
  static char ID;
  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;

  std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap;

  InstructionLatencyPass(TimingAnalysisResults &TAR);

  const std::unordered_map<const MachineBasicBlock *, unsigned int> &
  getMBBLatencyMap() const {
    return MBBLatencyMap;
  }
  std::unordered_map<const MachineBasicBlock *, unsigned int> &
  getMBBLatencyMap() {
    return MBBLatencyMap;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  bool runOnMachineFunction(MachineFunction &F) override;

  virtual llvm::StringRef getPassName() const override {
    return "ARM Timing Analysis Result Dump Pass";
  }
};

MachineFunctionPass *createInstructionLatencyPass(TimingAnalysisResults &TAR);
} // namespace llvm

#endif // LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H
