#include "Utility/InstructionWords.h"
#include "Targets/RTTarget.h"
#include "TimingAnalysisResults.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Target/TargetMachine.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace llvm {

std::unordered_map<const MachineInstr *, unsigned>
computeInstructionWords(const MachineFunction &MF,
                        const TimingAnalysisResults &TAR) {
  // Collect the resolved address of every instruction in this function so we
  // can derive each instruction's fetch length (in 16-bit words) from the gap
  // to the next address. Instructions without an address (debug/CFI pseudos,
  // or anything the resolver could not match) are simply skipped.
  std::vector<std::pair<uint64_t, const MachineInstr *>> Resolved;
  for (const auto &MBB : MF)
    for (const auto &MI : MBB)
      if (TAR.hasInstructionAddress(&MI))
        Resolved.push_back({TAR.getInstructionAddress(&MI), &MI});

  std::sort(Resolved.begin(), Resolved.end(),
            [](const auto &A, const auto &B) { return A.first < B.first; });

  // Word count per instruction = (next address - this address) / 2, capped to
  // the target's maximum instruction fetch width so a layout gap cannot inflate
  // the count. The last instruction has no successor address, so it falls back
  // to 1 word.
  const unsigned MaxWords = TAR.getTarget().getMaxInstructionWords();
  std::unordered_map<const MachineInstr *, unsigned> Words;
  for (size_t I = 0; I < Resolved.size(); ++I) {
    unsigned W = 1;
    if (I + 1 < Resolved.size()) {
      uint64_t Wd = (Resolved[I + 1].first - Resolved[I].first) / 2;
      if (Wd >= 1 && Wd <= MaxWords)
        W = static_cast<unsigned>(Wd);
    }
    Words[Resolved[I].second] = W;
  }
  return Words;
}

} // namespace llvm
