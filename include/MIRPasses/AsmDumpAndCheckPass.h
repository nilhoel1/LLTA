#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/LiveStacks.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "TimingAnalysisResults.h"

namespace llvm {

/**
 * Pass that prints the resulting assembler for the given program if option
 * enable-asm-dump is true. In any case, it checks that the program with its
 * instructions adheres to our implicit assumptions and gives reasonable error
 * messages to the user.
 */
class AsmDumpAndCheckPass : public MachineFunctionPass {
public:
  static char ID;
  TimingAnalysisResults &TAR;
  AsmDumpAndCheckPass(TimingAnalysisResults &TAR);

  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool doFinalization(Module &) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    // AU.addUsedIfAvailable<LiveStacks>();
    // AU.addUsedIfAvailable<LiveVariables>();
    // AU.addUsedIfAvailable<SlotIndexes>();
    // AU.addUsedIfAvailable<LiveIntervals>();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  virtual llvm::StringRef getPassName() const override {
    return "ARM Timing Analysis Result Dump Pass";
  }
  void checkMSP430Instruction(const MachineInstr &I);
};

} // namespace llvm

namespace llvm {
MachineFunctionPass *createAsmDumpAndCheckPass(TimingAnalysisResults &TAR);
} // namespace llvm
