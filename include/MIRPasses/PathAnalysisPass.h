#include "RTTargets/ProgramGraph.h"
#include "TimingAnalysisResults.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"

#include "Analysis/AbstractStateGraph.h"
#include "Analysis/GraphAdapter.h"
#include "Analysis/WorklistSolver.h"
#include "ILP/AbstractILPSolver.h"
#include "RTTargets/MSP430/MSP430Pipeline.h"

namespace llvm {

class PathAnalysisPass : public MachineFunctionPass {
public:
  static char ID;

  bool FoundStartingFunction = false;
  Function *StartingFunction = nullptr;

  const bool DebugPrints = true;
  TimingAnalysisResults &TAR;
  PathAnalysisPass(TimingAnalysisResults &TAR);

  CallGraph *CG = nullptr;

  AbstractStateGraph ASG;
  MSP430Pipeline Pipeline;
  WorklistSolver AnalysisWorker;
  std::unique_ptr<AbstractILPSolver> ABSolver;

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool dumpMuGraphToDotFile(ProgramGraph &MASG, StringRef FileName);
  Function *getStartingFunction(CallGraph &CG);

  bool finalizePathAnalysis(ProgramGraph &MASG);
  bool doFinalization(Module &) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<CallGraphWrapperPass>();
    AU.addRequired<SCEVAAWrapperPass>();
    AU.addRequired<MachineModuleInfoWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  virtual llvm::StringRef getPassName() const override {
    return "PathAnalysisPass for WCET computation via ILP";
  }
};
} // namespace llvm

namespace llvm {
MachineFunctionPass *createPathAnalysisPass(TimingAnalysisResults &TAR);
} // namespace llvm
