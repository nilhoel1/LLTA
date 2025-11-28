#ifndef LLVM_MIR_TO_IR_PASS_H
#define LLVM_MIR_TO_IR_PASS_H

#include "TimingAnalysisResults.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

namespace llvm {
class MIRtoIRPass : public MachineFunctionPass {
public:
  static char ID;

  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;
  MIRtoIRPass(TimingAnalysisResults &TAR);

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool doFinalization(Module &) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };
  virtual llvm::StringRef getPassName() const override {
    return "Map Machine Instructions to LLVM IR";
  }
};
} // namespace llvm

#endif // LLVM_MIR_TO_IR_PASS_H

namespace llvm {
MachineFunctionPass *createMIRtoIRPass(TimingAnalysisResults &TAR);
} // namespace llvm
