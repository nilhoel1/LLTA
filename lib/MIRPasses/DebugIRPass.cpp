#include "MIRPasses/DebugIRPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

char DebugIRPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
DebugIRPass::DebugIRPass()
    : MachineFunctionPass(ID){}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool DebugIRPass::doFinalization(Module &M) { return false; }

/**
 * @brief Iterates over MachineFunction
 *        and dumps its content into a File.
 *
 * @param F
 * @return true
 * @return false
 */
bool DebugIRPass::runOnMachineFunction(MachineFunction &F) {
  if (DebugPrints) {
    outs() << "MachineFunction: " << F.getName() << "\n";
  }


  for (auto &MBB : F) {
    for (auto &MI : MBB) {
    }
  }
  return false;
}


MachineFunctionPass *createDebugIRPass() {
  return new DebugIRPass();
}
} // namespace llvm
