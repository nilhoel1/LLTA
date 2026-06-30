#include "Utility/DataMemoryAccess.h"

#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace llvm {

/// True iff a single MachineMemOperand is provably a stack (SRAM) access.
static bool isStackMemOperand(const MachineMemOperand *MMO) {
  const PseudoSourceValue *PSV = MMO->getPseudoValue();
  if (!PSV)
    return false; // a real IR value (e.g. a global in FRAM) — not stack
  return PSV->isStack() || PSV->kind() == PseudoSourceValue::FixedStack;
}

/// True iff a single MachineMemOperand is provably to non-wait-state memory
/// (SRAM/stack), tightening isStackMemOperand with target-independent address
/// resolution. Beyond the stack pseudo-values, the originating IR object is
/// recovered (getValue -> getUnderlyingObject): an AllocaInst is stack/SRAM, and
/// a GlobalValue is SRAM iff its resolved address is below \p FRAMStart.
/// Conservative for soundness: anything not proven SRAM returns false.
static bool isProvablyNonFRAMMemOperand(const MachineMemOperand *MMO,
                                        uint64_t FRAMStart,
                                        GlobalAddressResolver Resolve) {
  if (isStackMemOperand(MMO))
    return true;
  const Value *V = MMO->getValue();
  if (!V)
    return false; // no IR object — unknown ⇒ assume FRAM
  const Value *Base = getUnderlyingObject(V);
  if (isa<AllocaInst>(Base))
    return true; // stack frame object ⇒ SRAM
  if (const auto *GV = dyn_cast<GlobalValue>(Base)) {
    if (std::optional<uint64_t> Addr = Resolve(*GV))
      return *Addr < FRAMStart; // below the FRAM region ⇒ SRAM
  }
  return false; // unknown global / computed pointer ⇒ assume FRAM
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

unsigned framDataAccessWords(const MachineInstr &MI, uint64_t FRAMStart,
                             GlobalAddressResolver Resolve) {
  if (!MI.mayLoad() && !MI.mayStore())
    return 0;

  // Unknown address (no memoperand info) ⇒ assume FRAM ⇒ charge the access(es).
  if (MI.memoperands_empty())
    return (MI.mayLoad() ? 1u : 0u) + (MI.mayStore() ? 1u : 0u);

  // Charge each operand not proven to target non-wait-state memory.
  unsigned Count = 0;
  for (const MachineMemOperand *MMO : MI.memoperands())
    if (!isProvablyNonFRAMMemOperand(MMO, FRAMStart, Resolve))
      ++Count;
  return Count;
}

} // namespace llvm
