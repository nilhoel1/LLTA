#ifndef LLTA_ANALYSIS_PIPELINEANALYSIS_H
#define LLTA_ANALYSIS_PIPELINEANALYSIS_H

#include "Analysis/HardwareStrategies.h"
#include "Analysis/SystemState.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetSchedule.h"

namespace llta {

class PipelineAnalysis {
  llvm::TargetSchedModel SchedModel;

  // Owned strategies (populated via factory or config)
  std::unique_ptr<CacheStrategy> ICache;
  std::unique_ptr<BranchPredictorStrategy> BPredictor;

public:
  PipelineAnalysis(const llvm::TargetSubtargetInfo &STI);

  SystemState getEntryState() const;
  SystemState getBottomState() const;

  /// The main Transfer Function.
  /// Computes State_Out = f(State_In, Instruction)
  SystemState transfer(const llvm::MachineInstr &MI, SystemState InState);

private:
  /// Queries LLVM SchedModel for structural hazards and basic latency.
  unsigned computeBaseLatency(const llvm::MachineInstr &MI, SystemState &State);

  /// Queries strategies for dynamic penalties (Cache Miss, Branch Flush).
  unsigned computeDynamicPenalties(const llvm::MachineInstr &MI);
};

} // namespace llta

#endif