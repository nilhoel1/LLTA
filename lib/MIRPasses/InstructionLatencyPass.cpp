#include "MIRPasses/InstructionLatencyPass.h"
#include <cassert>
#include <utility>

#include "Targets/RTTarget.h"
#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

char InstructionLatencyPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
InstructionLatencyPass::InstructionLatencyPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR),
      MBBLatencyMap(
          std::unordered_map<const MachineBasicBlock *, unsigned int>()) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
// bool InstructionLatencyPass::doFinalization(Module &M) { return false; }

/**
 * @brief Iterates over MachineFunction
 *        and dumps its content into a File.
 *
 * @param F
 * @return true
 * @return false
 */
bool InstructionLatencyPass::runOnMachineFunction(MachineFunction &F) {
  if (DebugPrints)
    outs() << "Running InstructionLatencyPass on Function: " << F.getName()
           << "\n";
  const llta::RTTarget &Target = TAR.getTarget();
  for (auto &MBB : F) {
    // Sum up the latencies of all instructions in the basic block
    unsigned int Latency = 0;
    for (auto &MI : MBB) {
      // Meta instructions (DBG_*, CFI, KILL, IMPLICIT_DEF, ...) carry no timing.
      if (MI.isMetaInstruction())
        continue;
      unsigned int InstructionLatency = Target.getInstructionLatency(MI);
      if (DebugPrints)
        outs() << "Instruction: " << MI << "Latency: " << InstructionLatency
               << "\n";
      Latency += InstructionLatency;
    }
    std::pair<const MachineBasicBlock *, unsigned int> MBBLatencyPair =
        std::make_pair(&MBB, Latency);
    auto NoDuplicate = MBBLatencyMap.insert(MBBLatencyPair);
    if (!NoDuplicate.second) {
      // If the pair already exists, print a warning
      errs() << "Warning: Duplicate MBB found: " << MBB.getName()
             << " with Latency: " << Latency << "\n";
    }
    assert(NoDuplicate.second && "Duplicate MBB found in MBBLatencyMap");
  }
  TAR.setMBBLatencyMap(MBBLatencyMap);
  return false;
}


MachineFunctionPass *createInstructionLatencyPass(TimingAnalysisResults &TAR) {
  return new InstructionLatencyPass(TAR);
}
} // namespace llvm
