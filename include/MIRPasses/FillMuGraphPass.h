#ifndef FILL_MU_GRAPH_PASS_H
#define FILL_MU_GRAPH_PASS_H

#include "RTTargets/ProgramGraph.h"
#include "TimingAnalysisResults.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

class FillMuGraphPass : public MachineFunctionPass {
public:
  static char ID;
  TimingAnalysisResults &TAR;
  bool FoundStartingFunction = false;
  Function *StartingFunction = nullptr;
  CallGraph *CG = nullptr;

  FillMuGraphPass(TimingAnalysisResults &TAR);

  bool runOnMachineFunction(MachineFunction &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  Function *getStartingFunction(CallGraph &CG);
};

MachineFunctionPass *createFillMuGraphPass(TimingAnalysisResults &TAR);

} // namespace llvm

#endif // FILL_MU_GRAPH_PASS_H
