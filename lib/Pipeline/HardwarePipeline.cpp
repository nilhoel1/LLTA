#include "Pipeline/HardwarePipeline.h"

namespace llvm {

void HardwarePipeline::injectInstruction(const MachineInstr *MI) {
  if (Stages.empty())
    return;

  // Inject into the first stage.
  // The first stage's execute() handles internal readiness/stalling.
  Stages.front()->execute(MI);
}

void HardwarePipeline::cycle() {
  if (Stages.empty())
    return;

  // Process stages in reverse order (last to first).
  // This ensures an instruction moves forward without collisions.
  for (size_t I = Stages.size(); I > 0; --I) {
    size_t Idx = I - 1;

    // Check if this stage can pass its instruction to the next stage.
    if (Idx + 1 < Stages.size() && !Stages[Idx]->isEmpty() &&
        Stages[Idx + 1]->isReady()) {
      const MachineInstr *MI = Stages[Idx]->getCurrentInstruction();
      Stages[Idx + 1]->execute(MI);
    }

    // If this is the last stage and it's finishing, track the retired instr.
    if (Idx == Stages.size() - 1 && !Stages[Idx]->isEmpty()) {
      // Will be marked as retired after cycle() completes for this stage.
      LastRetiredInstruction = Stages[Idx]->getCurrentInstruction();
    }

    Stages[Idx]->cycle();
  }
}

bool HardwarePipeline::isEmpty() const {
  for (const auto &Stage : Stages) {
    if (!Stage->isEmpty())
      return false;
  }
  return true;
}

bool HardwarePipeline::isRetired(const MachineInstr *MI) const {
  return LastRetiredInstruction == MI;
}

unsigned HardwarePipeline::convertCyclesToFastForward() const {
  unsigned MinBusy = 0;
  bool FoundBusy = false;

  for (const auto &Stage : Stages) {
    unsigned Busy = Stage->getBusyCycles();
    if (Busy > 0) {
      if (!FoundBusy || Busy < MinBusy) {
        MinBusy = Busy;
        FoundBusy = true;
      }
    }
  }

  return MinBusy;
}

HardwarePipeline HardwarePipeline::clone() const {
  HardwarePipeline NewPipeline;
  for (const auto &Stage : Stages) {
    NewPipeline.Stages.push_back(Stage->clone());
  }
  NewPipeline.LastRetiredInstruction = LastRetiredInstruction;
  return NewPipeline;
}

} // namespace llvm
