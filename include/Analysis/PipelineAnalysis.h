#ifndef LLTA_ANALYSIS_PIPELINEANALYSIS_H
#define LLTA_ANALYSIS_PIPELINEANALYSIS_H

#include "Analysis/HardwareStrategies.h"
#include "Analysis/SystemState.h"
#include "llta/Analysis/AbstractAnalysis.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetSchedule.h"

namespace llta {

/// PipelineAnalysis implements the AbstractAnalysis interface for WCET
/// analysis. It computes the worst-case execution time by modeling the
/// pipeline behavior and applying transfer functions to instructions.
class PipelineAnalysis : public AbstractAnalysis {
  llvm::TargetSchedModel SchedModel;

  // Owned strategies (populated via factory or config)
  std::unique_ptr<CacheStrategy> ICache;
  std::unique_ptr<BranchPredictorStrategy> BPredictor;

public:
  PipelineAnalysis(const llvm::TargetSubtargetInfo &STI);

  // AbstractAnalysis interface implementation
  std::unique_ptr<AbstractState>
  transfer(const AbstractState &FromState,
           const llvm::MachineInstr &MI) override;
  std::unique_ptr<AbstractState> join(const AbstractState &S1,
                                      const AbstractState &S2) override;
  std::unique_ptr<AbstractState> getInitialState() override;
  bool isLessOrEqual(const AbstractState &S1,
                     const AbstractState &S2) const override;

  // Legacy interface for backward compatibility
  SystemState getEntryState() const;
  SystemState getBottomState() const;

  /// The main Transfer Function (concrete version for direct use).
  /// Computes State_Out = f(State_In, Instruction)
  SystemState transferState(const llvm::MachineInstr &MI, SystemState InState);

private:
  /// Queries LLVM SchedModel for structural hazards and basic latency.
  unsigned computeBaseLatency(const llvm::MachineInstr &MI, SystemState &State);

  /// Queries strategies for dynamic penalties (Cache Miss, Branch Flush).
  unsigned computeDynamicPenalties(const llvm::MachineInstr &MI);
};

} // namespace llta

#endif