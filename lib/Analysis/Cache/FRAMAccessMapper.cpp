#include "Analysis/Cache/FRAMAccessMapper.h"
#include "TimingAnalysisResults.h"

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"

#include <cstdint>

namespace llvm {

/// True iff every memory access of \p MI is provably to the stack (SRAM), which
/// is never cached and therefore cannot perturb the FRAM cache. Conservative:
/// returns false (⇒ caller emits a Barrier) whenever any access cannot be
/// proven to be stack, including when no MachineMemOperand info is attached.
static bool isProvablyStackOnly(const MachineInstr &MI) {
  if (MI.memoperands_empty())
    return false;
  for (const MachineMemOperand *MMO : MI.memoperands()) {
    const PseudoSourceValue *PSV = MMO->getPseudoValue();
    if (!PSV)
      return false; // a real IR value (e.g. a global in FRAM) — not stack
    if (!(PSV->isStack() || PSV->kind() == PseudoSourceValue::FixedStack))
      return false;
  }
  return true;
}

void FRAMAccessMapper::mapEvents(const MachineInstr *MI,
                                 SmallVectorImpl<CacheEvent> &Out) {
  // 1. Instruction fetch: one access per 16-bit code word in FRAM.
  auto It = Words.find(MI);
  if (It != Words.end() && TAR.hasInstructionAddress(MI)) {
    const uint64_t Addr = TAR.getInstructionAddress(MI);
    const uint64_t FramStart = TAR.getFRAMStart();
    for (unsigned W = 0; W < It->second; ++W) {
      uint64_t WordAddr = Addr + 2ULL * W;
      if (WordAddr >= FramStart)
        Out.push_back(CacheEvent::access(Geo.lineId(WordAddr)));
    }
  }

  // 2. Data access: SRAM/stack is transparent; anything else is a barrier.
  if (MI->mayLoad() || MI->mayStore()) {
    if (!isProvablyStackOnly(*MI))
      Out.push_back(CacheEvent::barrier());
  }
}

} // namespace llvm
