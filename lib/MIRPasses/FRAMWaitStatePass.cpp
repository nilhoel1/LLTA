#include "MIRPasses/FRAMWaitStatePass.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llvm {

char FRAMWaitStatePass::ID = 0;

FRAMWaitStatePass::FRAMWaitStatePass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

bool FRAMWaitStatePass::runOnMachineFunction(MachineFunction &F) {
  // No-op unless the model is configured and the FRAM region is known. This
  // keeps default runs (and the regression tests) byte-for-byte unchanged.
  if (FRAMWaitStates == 0 || !TAR.hasFRAMStart())
    return false;

  const uint64_t FramStart = TAR.getFRAMStart();

  // Collect the resolved address of every instruction in this function so we
  // can derive each instruction's fetch length (in 16-bit words) from the gap
  // to the next address. Instructions without an address (debug/CFI pseudos,
  // or anything the resolver could not match) are simply skipped.
  std::vector<std::pair<uint64_t, const MachineInstr *>> Resolved;
  for (auto &MBB : F)
    for (auto &MI : MBB)
      if (TAR.hasInstructionAddress(&MI))
        Resolved.push_back({TAR.getInstructionAddress(&MI), &MI});

  if (Resolved.empty())
    return false;

  std::sort(Resolved.begin(), Resolved.end(),
            [](const auto &A, const auto &B) { return A.first < B.first; });

  // Word count per instruction = (next address - this address) / 2, capped to
  // the MSP430 maximum of 3 words so a layout gap cannot inflate the penalty.
  // The last instruction has no successor address, so it falls back to 1 word.
  std::unordered_map<const MachineInstr *, unsigned> Words;
  for (size_t I = 0; I < Resolved.size(); ++I) {
    unsigned W = 1;
    if (I + 1 < Resolved.size()) {
      uint64_t Wd = (Resolved[I + 1].first - Resolved[I].first) / 2;
      if (Wd >= 1 && Wd <= 3)
        W = static_cast<unsigned>(Wd);
    }
    Words[Resolved[I].second] = W;
  }

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
