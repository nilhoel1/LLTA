#ifndef MICRO_ARCHITECTURE_ANALYSIS_H
#define MICRO_ARCHITECTURE_ANALYSIS_H

#include "../Pipeline/HardwarePipeline.h"
#include "AbstractAnalysable.h"
#include "AbstractState.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <memory>
#include <string>

namespace llvm {

/**
 * Abstract state for microarchitecture analysis.
 * Holds the current pipeline state.
 */
class MicroArchState : public AbstractState {
public:
  MicroArchState() = default;

  explicit MicroArchState(HardwarePipeline Pipeline)
      : Pipeline(std::move(Pipeline)) {}

  std::unique_ptr<AbstractState> clone() const override {
    return std::make_unique<MicroArchState>(Pipeline.clone());
  }

  bool equals(const AbstractState *Other) const override {
    // For WCET analysis, equality check can be conservative.
    // Two states are equal if both pipelines are empty.
    const auto *OtherState = dynamic_cast<const MicroArchState *>(Other);
    if (!OtherState)
      return false;
    return Pipeline.isEmpty() && OtherState->Pipeline.isEmpty();
  }

  bool join(const AbstractState *Other) override {
    // For WCET, join is typically a no-op if we want max cycles.
    // A more sophisticated approach would join pipeline contents.
    // For now, return false (no change).
    return false;
  }

  std::string toString() const override {
    return Pipeline.isEmpty() ? "MicroArchState{empty}"
                              : "MicroArchState{active}";
  }

  HardwarePipeline &getPipeline() { return Pipeline; }
  const HardwarePipeline &getPipeline() const { return Pipeline; }

private:
  HardwarePipeline Pipeline;
};

/**
 * Analysis wrapper that integrates HardwarePipeline into AbstractAnalysable.
 */
class MicroArchitectureAnalysis : public AbstractAnalysable {
public:
  /**
   * Construct with an initial pipeline configuration.
   */
  explicit MicroArchitectureAnalysis(HardwarePipeline InitialPipeline)
      : InitialPipeline(std::move(InitialPipeline)) {}

  std::unique_ptr<AbstractState> getInitialState() override {
    return std::make_unique<MicroArchState>(InitialPipeline.clone());
  }

  /**
   * Process an instruction through the pipeline.
   * Returns the number of cycles consumed until the instruction retires.
   */
  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    auto *MicroState = dynamic_cast<MicroArchState *>(State);
    if (!MicroState)
      return 0;

    HardwarePipeline &Pipeline = MicroState->getPipeline();
    unsigned TotalCycles = 0;

    // Inject the instruction into the pipeline.
    Pipeline.injectInstruction(MI);

    // Cycle until this instruction retires.
    while (!Pipeline.isRetired(MI)) {
      // Attempt fast-forward optimization.
      unsigned SkipCycles = Pipeline.convertCyclesToFastForward();
      if (SkipCycles > 1) {
        // Fast-forward: advance multiple cycles at once.
        // Note: A full implementation would call cycle() SkipCycles times
        // or have a dedicated advanceCycles(n) method.
        for (unsigned I = 0; I < SkipCycles; ++I) {
          Pipeline.cycle();
        }
        TotalCycles += SkipCycles;
      } else {
        Pipeline.cycle();
        ++TotalCycles;
      }
    }

    return TotalCycles;
  }

private:
  HardwarePipeline InitialPipeline;
};

} // namespace llvm

#endif // MICRO_ARCHITECTURE_ANALYSIS_H
