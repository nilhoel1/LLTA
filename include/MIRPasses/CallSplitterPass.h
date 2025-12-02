#include "TimingAnalysisResults.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
namespace llvm {

/**
 * Pass that prints the resulting assembler for the given program if option
 * enable-asm-dump is true. In any case, it checks that the program with its
 * instructions adheres to our implicit assumptions and gives reasonable error
 * messages to the user.
 */
class CallSplitterPass : public MachineFunctionPass {
public:
  static char ID;

  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;
  CallSplitterPass(TimingAnalysisResults &TAR);

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool doFinalization(Module &) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<CallGraphWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  virtual llvm::StringRef getPassName() const override {
    return "CallSplitterPass, splitting all BB wiht function calls into three  "
           "BBs";
  }
};

} // namespace llvm

namespace llvm {
MachineFunctionPass *createCallSplitterPass(TimingAnalysisResults &TAR);
} // namespace llvm
