#include "Targets/MSP430/FRAMWaitStatePass.h"
#include "Targets/MSP430/MSP430Options.h"
#include "TimingAnalysisResults.h"
#include "Utility/InstructionWords.h"
#include "Utility/Options.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <unordered_map>

namespace llvm {

char FRAMWaitStatePass::ID = 0;

FRAMWaitStatePass::FRAMWaitStatePass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

bool FRAMWaitStatePass::runOnMachineFunction(MachineFunction &F) {
  // No-op unless the model is configured and the FRAM region is known. This
  // keeps default runs (and the regression tests) byte-for-byte unchanged.
  // Also a no-op when the FRAM cache analysis is enabled: FRAMCacheAnalysisPass
  // then owns the FRAM fetch penalty (cache-aware), and running both would
  // double-count.
  if (FRAMWaitStates == 0 || !TAR.hasFRAMStart() || FRAMCache)
    return false;

  const uint64_t FramStart = TAR.getFRAMStart();

  // Per-instruction 16-bit fetch word counts (shared with the cache analysis).
  std::unordered_map<const MachineInstr *, unsigned> Words =
      computeInstructionWords(F, TAR);
  if (Words.empty())
    return false;

  // Read-modify-write the accumulated MBBLatencyMap. InstructionLatencyPass
  // runs earlier in the pipeline for the same function, so an entry already
  // exists for every MBB of F.
  auto Map = TAR.getMBBLatencyMap();
  unsigned FuncPenalty = 0;
  for (auto &MBB : F) {
    unsigned Penalty = 0;
    for (auto &MI : MBB) {
      auto It = Words.find(&MI);
      if (It == Words.end())
        continue;
      if (TAR.getInstructionAddress(&MI) >= FramStart)
        Penalty += FRAMWaitStates * It->second;
    }
    if (Penalty) {
      Map[&MBB] += Penalty;
      FuncPenalty += Penalty;
    }
  }
  TAR.setMBBLatencyMap(Map);

  if (DebugPrints || AddressResolverVerbose)
    outs() << "[fram-wait] " << F.getName() << ": +" << FuncPenalty
           << " cycle(s) at " << FRAMWaitStates << " wait state(s)/access\n";

  return false;
}

MachineFunctionPass *createFRAMWaitStatePass(TimingAnalysisResults &TAR) {
  return new FRAMWaitStatePass(TAR);
}

} // namespace llvm
