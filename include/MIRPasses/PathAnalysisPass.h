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
 * Pass that prints the resulting assembler for the given program if option
 * enable-asm-dump is true. In any case, it checks that the program with its
 * instructions adheres to our implicit assumptions and gives reasonable error
 * messages to the user.
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
    return "PathAnalysisPass for testing different analysis results";
  }
};

} // namespace llvm
namespace llvm {
MachineFunctionPass *createPathAnalysisPass(TimingAnalysisResults &TAR);
} // namespace llvm
