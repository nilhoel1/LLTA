#include "MIRPasses/AsmDumpAndCheckPass.h"
#include "Targets/RTTarget.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include <cassert>

namespace llvm {

char AsmDumpAndCheckPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
AsmDumpAndCheckPass::AsmDumpAndCheckPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool AsmDumpAndCheckPass::doFinalization(Module &M) { return false; }

/**
 * @brief Iterates over MachineFunction
 *        and dumps its content into a File.
 *
 * @param F
 * @return true
 * @return false
 */
bool AsmDumpAndCheckPass::runOnMachineFunction(MachineFunction &F) {
  const llta::RTTarget &Target = TAR.getTarget();
  for (auto &MBB : F)
    for (auto &MI : MBB) {
      // Meta instructions (DBG_*, CFI, KILL, IMPLICIT_DEF, ...) carry no timing
      // and are not part of any target's instruction model.
      if (MI.isMetaInstruction())
        continue;
      Target.checkInstruction(MI);
    }
  return false;
}

MachineFunctionPass *createAsmDumpAndCheckPass(TimingAnalysisResults &TAR) {
  return new AsmDumpAndCheckPass(TAR);
}
} // namespace llvm
