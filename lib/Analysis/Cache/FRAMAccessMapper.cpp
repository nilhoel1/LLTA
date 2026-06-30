#include "Analysis/Cache/FRAMAccessMapper.h"
#include "TimingAnalysisResults.h"
#include "Utility/DataMemoryAccess.h"

#include "llvm/CodeGen/MachineInstr.h"

#include <cstdint>

namespace llvm {

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

  // 2. Data access: a provably stack/SRAM access is transparent (0 words);
  //    anything not proven non-wait-state is assumed FRAM — emit a Barrier that
  //    both wipes the abstract cache state and charges the wait-state cost.
  if (unsigned N = framDataAccessWords(*MI))
    Out.push_back(CacheEvent::barrier(DataAccessCost * N));
}

} // namespace llvm
