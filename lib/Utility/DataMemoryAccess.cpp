#include "Utility/DataMemoryAccess.h"

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"

namespace llvm {

/// True iff a single MachineMemOperand is provably a stack (SRAM) access.
static bool isStackMemOperand(const MachineMemOperand *MMO) {
  const PseudoSourceValue *PSV = MMO->getPseudoValue();
  if (!PSV)
    return false; // a real IR value (e.g. a global in FRAM) — not stack
  return PSV->isStack() || PSV->kind() == PseudoSourceValue::FixedStack;
}

bool isProvablyStackOnly(const MachineInstr &MI) {
  if (MI.memoperands_empty())
    return false;
  for (const MachineMemOperand *MMO : MI.memoperands())
    if (!isStackMemOperand(MMO))
      return false;
  return true;
}

unsigned framDataAccessWords(const MachineInstr &MI) {
  if (!MI.mayLoad() && !MI.mayStore())
    return 0;
  if (isProvablyStackOnly(MI))
    return 0;

  // Unknown address (no memoperand info) ⇒ assume FRAM ⇒ charge the access(es).
  if (MI.memoperands_empty())
    return (MI.mayLoad() ? 1u : 0u) + (MI.mayStore() ? 1u : 0u);

  // Charge each operand that is not provably stack.
  unsigned Count = 0;
  for (const MachineMemOperand *MMO : MI.memoperands())
    if (!isStackMemOperand(MMO))
      ++Count;
  return Count;
}

} // namespace llvm
