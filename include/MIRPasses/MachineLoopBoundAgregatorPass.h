#ifndef MACHINE_LOOP_BOUND_AGREGATOR_PASS_H
#define MACHINE_LOOP_BOUND_AGREGATOR_PASS_H

#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

namespace llvm {

class MachineLoopBoundAgregatorPass : public MachineFunctionPass {
public:
  static char ID;
  TimingAnalysisResults &TAR;

  MachineLoopBoundAgregatorPass(TimingAnalysisResults &TAR);

  bool runOnMachineFunction(MachineFunction &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

MachineFunctionPass *createMachineLoopBoundAgregatorPass(TimingAnalysisResults &TAR);

} // namespace llvm

#endif // MACHINE_LOOP_BOUND_AGREGATOR_PASS_H
