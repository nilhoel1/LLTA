#include "TimingAnalysisResults.h"
#include "RTTargets/MuArchStateGraph.h"
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

//#include "MIRPasses/InstructionLatencyPass.h"
namespace llvm {

/**
 * Pass that performs path analysis on the MuArchStateGraph to compute WCET.
 * This pass solves an ILP to find the longest path from entry to exit node.
 */
class PathAnalysisPass : public MachineFunctionPass {
public:
  static char ID;

  bool FoundStartingFunction = false;

  Function *StartingFunction = nullptr;

  // a struct that holds graphnodes and edges, which link to BBs
  // and instructions

  const bool DebugPrints = true;
  TimingAnalysisResults &TAR;
  PathAnalysisPass(TimingAnalysisResults &TAR);

  CallGraph *CG = nullptr;

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool dumpMuGraphToDotFile(MuArchStateGraph &MASG, StringRef FileName);
  Function *getStartingFunction(CallGraph &CG);

  /// Finalize the path analysis by solving the WCET ILP
  /// @param MASG The microarchitectural state graph to analyze
  /// @return true if WCET was successfully computed
  bool finalizePathAnalysis(MuArchStateGraph &MASG);

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
