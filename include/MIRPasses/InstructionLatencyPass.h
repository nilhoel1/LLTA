#ifndef LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H
#define LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H

#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Target/TargetMachine.h"

#include <cstddef>
#include <unordered_map>

namespace llvm {

/**
 * This pass sums up the instruction Latency of each basic block in a
 * function. It is used to check if the instruction latencies are implemented.
 * TODO: The current implementation assumes that MSP430X is used.
 * It further assumes no Pipeline, which is true for the MSP430X.
 */
class InstructionLatencyPass : public MachineFunctionPass {
public:
  static char ID;
  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;

  std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap;

  InstructionLatencyPass(TimingAnalysisResults &TAR);

  const std::unordered_map<const MachineBasicBlock *, unsigned int> &getMBBLatencyMap() const {
    return MBBLatencyMap;
  }
  std::unordered_map<const MachineBasicBlock *, unsigned int> &getMBBLatencyMap() {
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

  unsigned int getMSP430Latency(const MachineInstr &I);
};


MachineFunctionPass *createInstructionLatencyPass(TimingAnalysisResults &TAR);
} // namespace llvm

#endif // LLVM_LLTA_MIRPASSES_INSTRUCTIONLATENCYPASS_H
