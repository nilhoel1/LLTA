#ifndef UTILITY_DATA_MEMORY_ACCESS_H
#define UTILITY_DATA_MEMORY_ACCESS_H

#include "llvm/ADT/STLFunctionalExtras.h"

#include <cstdint>
#include <optional>

namespace llvm {

class GlobalValue;
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

/// Resolves a global to its absolute load address, or std::nullopt when the
/// address is unknown (e.g. the symbol was not found in the linked ELF).
using GlobalAddressResolver =
    function_ref<std::optional<uint64_t>(const GlobalValue &)>;

/// As framDataAccessWords(MI), but tightens the classification with
/// target-independent address resolution: an access whose underlying object is
/// provably below \p FRAMStart (SRAM) — or a stack alloca — is non-wait-state
/// and not counted. This recovers the originating IR object from each
/// MachineMemOperand (getValue -> getUnderlyingObject) and, for a GlobalValue,
/// resolves its address via \p Resolve and compares it to \p FRAMStart.
///
/// Soundness is preserved: a memory operand is only dropped when *proven* to be
/// SRAM/stack. Every unproven case (no IR value, unknown global, address
/// >= FRAMStart, or a non-global/computed base) is charged exactly as the
/// single-argument overload would. With no resolvable globals (e.g. \p Resolve
/// always returns std::nullopt, as when no ELF was linked) the result is
/// identical to framDataAccessWords(MI).
unsigned framDataAccessWords(const MachineInstr &MI, uint64_t FRAMStart,
                             GlobalAddressResolver Resolve);

} // namespace llvm

#endif // UTILITY_DATA_MEMORY_ACCESS_H
