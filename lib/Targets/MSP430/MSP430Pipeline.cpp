#include "Targets/MSP430/MSP430Pipeline.h"
#include "Analysis/InstructionCacheAnalysis.h"
#include "Targets/MSP430/MSP430Target.h"
#include "llvm/CodeGen/MachineInstr.h"

namespace llvm {

MSP430Pipeline::MSP430Pipeline() {
  // Execution stage: models instruction latencies for MSP430 opcodes.
  addAnalysis(std::make_unique<SimpleStage>("Execution"));

  // Instruction cache analysis (dummy template — always reports a hit).
  //
  // The MSP430 has no hardware instruction cache, so the miss penalty is 0
  // and this analysis does not affect the WCET result. It is included here
  // to demonstrate how to plug a second analysis into the pipeline and to
  // serve as a starting point for targets that do have an instruction cache
  // (e.g. ESP32-C6). Replace the penalty argument and the process() body in
  // InstructionCacheAnalysis.h with a real model when needed.
  addAnalysis(std::make_unique<InstructionCacheAnalysis>(/*MissPenalty=*/0));
}

std::unique_ptr<AbstractState> SimpleStage::getInitialState() {
  return std::make_unique<SimpleStageState>(0);
}

unsigned SimpleStage::process(AbstractState *State, const MachineInstr *MI) {
  auto *SState = static_cast<SimpleStageState *>(State);
  unsigned Latency = llta::getMSP430Latency(*MI);
  SState->Val += Latency;
  return Latency;
}

} // namespace llvm
