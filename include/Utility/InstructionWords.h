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

} // namespace llvm

#endif // UTIL_INSTRUCTION_WORDS_H
