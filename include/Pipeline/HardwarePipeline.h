#ifndef LLTA_PIPELINE_HARDWARE_PIPELINE_H
#define LLTA_PIPELINE_HARDWARE_PIPELINE_H

#include "llvm/CodeGen/MachineInstr.h"
#include <memory>
#include <vector>

namespace llvm {

/**
 * Interface for a hardware pipeline stage.
 * Concrete implementations model specific stages (Fetch, Decode, Execute, etc.)
 */
class AbstractHardwareStage {
public:
  virtual ~AbstractHardwareStage() = default;

  /**
   * Advance the stage by one clock cycle.
   */
  virtual void cycle() = 0;

  /**
   * Check if this stage is ready to accept an instruction or pass to next.
   */
  virtual bool isReady() const = 0;

  /**
   * Process an instruction entering this stage.
   */
  virtual void execute(const MachineInstr *MI) = 0;

  /**
   * Clone this stage (for state cloning).
   */
  virtual std::unique_ptr<AbstractHardwareStage> clone() const = 0;

  /**
   * Returns the number of cycles this stage will remain busy.
   * Used for fast-forwarding the simulation.
   * Returns 0 if idle or if busy time is unknown.
   */
  virtual unsigned getBusyCycles() const = 0;

  /**
   * Check if the stage currently holds an instruction.
   */
  virtual bool isEmpty() const = 0;

  /**
   * Get the instruction currently in the stage (if any).
   */
  virtual const MachineInstr *getCurrentInstruction() const = 0;
};

/**
 * A cycle-accurate pipeline model.
 * Holds a sequence of hardware stages and simulates instruction flow.
 */
class HardwarePipeline {
public:
  HardwarePipeline() = default;

  /**
   * Add a stage to the pipeline.
   * Stages should be added in order (Fetch -> Decode -> Execute -> ...).
   */
  void addStage(std::unique_ptr<AbstractHardwareStage> Stage) {
    Stages.push_back(std::move(Stage));
  }

  /**
   * Inject an instruction into the first stage of the pipeline.
   * Handles stalling if the first stage is not ready.
   */
  void injectInstruction(const MachineInstr *MI);

  /**
   * Advance all stages by one clock cycle (reverse order: Execute -> Fetch).
   */
  void cycle();

  /**
   * Check if all stages are empty.
   */
  bool isEmpty() const;

  /**
   * Check if a specific instruction has retired (left the last stage).
   */
  bool isRetired(const MachineInstr *MI) const;

  /**
   * Calculate the minimum number of cycles to fast-forward.
   * Based on the minimum getBusyCycles() across all non-idle stages.
   */
  unsigned convertCyclesToFastForward() const;

  /**
   * Deep clone the pipeline state.
   */
  HardwarePipeline clone() const;

  /**
   * Get the number of stages.
   */
  size_t getNumStages() const { return Stages.size(); }

private:
  std::vector<std::unique_ptr<AbstractHardwareStage>> Stages;

  /// Tracks the last instruction that retired.
  const MachineInstr *LastRetiredInstruction = nullptr;
};

} // namespace llvm

#endif // LLTA_PIPELINE_HARDWARE_PIPELINE_H
