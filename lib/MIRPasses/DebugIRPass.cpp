#include "MIRPasses/DebugIRPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/Support/Format.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include <string>
#include <iterator>
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"

namespace llvm {

char DebugIRPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
DebugIRPass::DebugIRPass() : MachineFunctionPass(ID) {}

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

  MachineRegisterInfo &MRI = F.getRegInfo();

  Module &M = *F.getFunction().getParent();
  NamedMDNode *CUs = M.getNamedMetadata("llvm.dbg.cu");
  DICompileUnit *CU = nullptr;
  if (CUs && CUs->getNumOperands() > 0) {
    CU = cast<DICompileUnit>(CUs->getOperand(0));
  }
  DIBuilder DIB(M, true, CU);
  DISubprogram *SP = F.getFunction().getSubprogram();
  DIFile *File = nullptr;
  DIType *Type = nullptr;
  const TargetInstrInfo *TII = nullptr;
  if (SP) {
    File = SP->getFile();
    Type = DIB.createBasicType("int", 32, dwarf::DW_ATE_signed);
    TII = F.getSubtarget().getInstrInfo();
  }

  for (auto &MBB : F) {
    outs() << " MBB #" << MBB.getNumber() << "\n";
    for (auto &MI : MBB) {
      // Print the instruction itself (assembly-like form)
      outs() << "  " << MI;

      // Try to print debug location if present
      DebugLoc DL = MI.getDebugLoc();
      if (DL) {
        if (const DILocation *Loc = DL.get()) {
          outs() << "    ; src: " << Loc->getFilename() << ":" << Loc->getLine();
        }
      }
      outs() << "\n";

      // For each register operand, print def/use information for virtual registers
      for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
        const MachineOperand &MO = MI.getOperand(i);
        if (!MO.isReg())
          continue;

        unsigned Reg = MO.getReg();
        if (llvm::Register::isVirtualRegister(Reg)) {
          MachineInstr *DefMI = MRI.getVRegDef(Reg);
          outs() << "    op[" << i << "] vreg %" << Reg << ": ";
          if (DefMI) {
            // Print a one-line summary of the defining instruction
            std::string DefStr;
            raw_string_ostream RSO(DefStr);
            RSO << *DefMI;
            RSO.str();
            // Collapse newlines for compactness
            for (char &c : DefStr)
              if (c == '\n')
                c = ' ';
            outs() << DefStr << "\n";
          } else {
            outs() << "<undef>\n";
          }
        } else {
          outs() << "    op[" << i << "] phys-reg " << Reg << "\n";
        }
      }

      // Insert DBG_VALUE for defined virtual registers
      if (SP) {
        for (unsigned i = 0; i < MI.getNumOperands(); ++i) {
          const MachineOperand &MO = MI.getOperand(i);
          if (MO.isReg() && MO.isDef() && Register::isVirtualRegister(MO.getReg())) {
            unsigned Reg = MO.getReg();
            DILocalVariable *Var = DIB.createAutoVariable(SP, "unnamed_" + std::to_string(UnnamedCounter++), File, 0, Type, true);
            DIExpression *Expr = DIB.createExpression();
            BuildMI(MBB, std::next(MI.getIterator()), MI.getDebugLoc(), TII->get(TargetOpcode::DBG_VALUE))
              .addReg(Reg)
              .addImm(0)
              .addMetadata(Var)
              .addMetadata(Expr);
          }
        }
      }
    }
  }

  if (SP) {
    DIB.finalize();
  }

  return false;
}



MachineFunctionPass *createDebugIRPass() { return new DebugIRPass(); }
} // namespace llvm
