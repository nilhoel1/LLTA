#ifndef UTILITY_DATA_MEMORY_ACCESS_H
#define UTILITY_DATA_MEMORY_ACCESS_H

namespace llvm {

class MachineInstr;

/// True iff every memory access of \p MI is provably to the stack (SRAM), which
/// on the MSP430FR family is never an FRAM wait-state access and cannot perturb
/// the FRAM instruction-fetch cache. Conservative for soundness: returns false
/// whenever any access cannot be proven to be stack — including when no
/// MachineMemOperand info is attached (unknown address ⇒ assume FRAM).
bool isProvablyStackOnly(const MachineInstr &MI);

/// Number of data memory accesses of \p MI that must be charged the FRAM
/// data-access wait-state cost, i.e. accesses that cannot be proven to target
/// non-wait-state memory (SRAM/stack). Soundness rule: unknown ⇒ assume FRAM ⇒
/// charge.
///
///   - 0 if \p MI is provably stack-only (or makes no data access);
///   - otherwise the count of MachineMemOperands not provably stack (>= 1);
///   - if \p MI has no MachineMemOperand info but may load/store, the access
///     target is unknown, so it is assumed FRAM and counted (load + store).
unsigned framDataAccessWords(const MachineInstr &MI);

} // namespace llvm

#endif // UTILITY_DATA_MEMORY_ACCESS_H
