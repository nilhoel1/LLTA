#include "MIRPasses/CallSplitterPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

char CallSplitterPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
CallSplitterPass::CallSplitterPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool CallSplitterPass::doFinalization(Module &M) { return false; }

/**
 * @brief Iterates over MachineFunction
 *        and dumps its content into a File.
 *
 * @param F
 * @return true
 * @return false
 */
bool CallSplitterPass::runOnMachineFunction(MachineFunction &F) {
  if (DebugPrints) {
    outs() << "MachineFunction: " << F.getName() << "\n";
  }

  /*output basic blocks before splitting*/
  // unsigned Count = 0;
  // if(DebugPrints){
  //   outs() << "\nBefore Splitting:\n\n";
  //   for (auto &MBB : F){
  //     outs() << "MBB: " << MBB.getName() << ": \n";
  //     for (auto &MI : MBB) {
  //       outs() << "MI: " << MI << " -";
  //     }
  //     outs() << "\n";
  //     Count++;
  //   }
  // }


  /*split basic blocks before and after calls*/
  for (auto &MBB : F) {

    /*split block after call*/
    for (auto &MI : MBB) {
      if (MI.isCall()){
        MBB.splitAt(MI);
      }
    }

    /*get position of instruction before call (if present)*/
    // int PositionBeforeCall = -1;
    // int Counter = 0;

    // for (auto &MI : MBB) {
    //   if (MI.isCall()){
    //     PositionBeforeCall = Counter - 1;
    //   }
    //   ++Counter;
    // }

    // /*split block before call, if a call exists and is not the first instruction*/
    // Counter = 0;
    // if (PositionBeforeCall >= 0){
    //   for (auto &MI : MBB) {
    //     if (Counter == PositionBeforeCall){
    //       MBB.splitAt(MI);
    //     }
    //     ++Counter;
    //   }
    // }

  }

  /*output basic blocks after splitting*/
  // if(DebugPrints){
  //   outs() << "\nNumber BB: " << Count;
  //   Count = 0;
  //   outs() << "\n\nAfter splitting:\n\n";

  //   for (auto &MBB : F){
  //     outs() << "MBB: " << MBB.getName() << ": \n";
  //     for (auto &MI : MBB) {
  //       outs() << "MI: " << MI << " -";
  //     }
  //     outs() << "\n";
  //     Count++;
  //   }
  //   outs() << "\nNumber BB: " << Count << "\n-----------------\n";
  // }


  return false;
}


MachineFunctionPass *createCallSplitterPass(TimingAnalysisResults &TAR) {
  return new CallSplitterPass(TAR);
}
} // namespace llvm
