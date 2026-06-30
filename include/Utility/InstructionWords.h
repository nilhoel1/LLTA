#ifndef UTIL_INSTRUCTION_WORDS_H
#define UTIL_INSTRUCTION_WORDS_H

#include <unordered_map>

namespace llvm {

class MachineFunction;
class MachineInstr;
class TimingAnalysisResults;

/// Compute, for each resolved code-emitting instruction of \p MF, the number of
/// 16-bit code words it fetches.
///
/// The word count is derived from the gap to the next resolved instruction
/// address (in \p TAR's InstructionAddressMap), capped to the active target's
/// maximum instruction fetch width (RTTarget::getMaxInstructionWords) so a
/// layout gap cannot inflate the count; the last resolved instruction of the
/// function (which has no successor address) falls back to 1 word. Instructions
/// without a resolved address are omitted from the result.
///
/// This is the shared basis for fetch-side memory penalties (e.g. a target's
/// wait-state / cache passes), so they agree on how many fetch accesses each
/// instruction performs.
std::unordered_map<const MachineInstr *, unsigned>
computeInstructionWords(const MachineFunction &MF,
                        const TimingAnalysisResults &TAR);

/// Compute, for each instruction of \p MF that makes a data memory access, the
/// number of access words that must be charged the FRAM data-access wait state
/// — i.e. accesses not provably to non-wait-state memory (SRAM/stack). This is
/// framDataAccessWords with target-independent address resolution: a global is
/// proven SRAM by looking its address up in \p TAR's ELF-derived DataObjects and
/// comparing to TAR's FRAM start. Only non-zero counts are stored, so a missing
/// entry means "no FRAM data charge". Resolution runs once per function, so the
/// cache fixpoint never recomputes it.
std::unordered_map<const MachineInstr *, unsigned>
computeDataAccessWords(const MachineFunction &MF,
                       const TimingAnalysisResults &TAR);

} // namespace llvm

#endif // UTIL_INSTRUCTION_WORDS_H
