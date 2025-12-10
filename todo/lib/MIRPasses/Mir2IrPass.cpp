#include "MIRPasses/Mir2IrPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

char MIRtoIRPass::ID = 0;

/**
 * @brief Construct a new Mir2IR Pass Object:: Mir2IR Pass
 *
 * @param TM
 */
MIRtoIRPass::MIRtoIRPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool MIRtoIRPass::doFinalization(Module &M) { return false; }

bool MIRtoIRPass::runOnMachineFunction(MachineFunction &MF) {
  // errs() << "Processing Machine Function: " << MF.getName() << "\n";

  MachineRegisterInfo &MRI = MF.getRegInfo(); // To track virtual registers

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (DebugPrints)
        errs() << "MachineInstr: " << MI << "\n";

      // Retrieve IR instruction using DebugLoc
      if (MI.getDebugLoc()) {
        DebugLoc DL = MI.getDebugLoc();
        const DILocation *DIL = DL.get();
        if (DIL) {
          if (DebugPrints)
            errs() << "  ↳ Corresponding LLVM IR Location: "
                   << DIL->getFilename() << ":" << DIL->getLine() << "\n";
        }
      } else {
        if (DebugPrints)
          errs() << "  ↳ No DebugLoc found\n";
      }

      // Track virtual register definitions back to IR
      for (const MachineOperand &MO : MI.operands()) {
        if (MO.isReg() && Register::isVirtualRegister(MO.getReg())) {
          Register VReg = MO.getReg();
          if (MachineInstr *DefMI = MRI.getVRegDef(VReg)) {
            errs() << "  ↳ Defined by Virtual Register in: " << *DefMI << "\n";
          }
        }
      }
    }
  }
  return false; // No modification to the MachineFunction
}

MachineFunctionPass *createMIRtoIRPass(TimingAnalysisResults &TAR) {
  return new MIRtoIRPass(TAR);
}
} // namespace llvm
// Register the pass
// static RegisterPass<MIRtoIRPass> X("mir-to-ir", "Map Machine Instructions to
// LLVM IR", false, false);
