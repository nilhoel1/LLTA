#ifndef ANALYSIS_PIPELINE_ANALYSIS_H
#define ANALYSIS_PIPELINE_ANALYSIS_H

#include "AbstractAnalysable.h"
#include <memory>
#include <vector>

namespace llvm {

/**
 * A composite analysis that runs multiple analyses in sequence.
 * This can represent a processor pipeline or just a set of independent
 * analyses.
 */
class PipelineAnalysis : public AbstractAnalysable {
public:
  void addAnalysis(std::unique_ptr<AbstractAnalysable> Analysis);

  std::unique_ptr<AbstractState> getInitialState() override;

  unsigned process(AbstractState *State, const MachineInstr *MI) override;

private:
  std::vector<std::unique_ptr<AbstractAnalysable>> Analyses;
};

/**
 * Composite State for the Pipeline.
 */
class PipelineState : public AbstractState {
public:
  std::vector<std::unique_ptr<AbstractState>> SubStates;

  PipelineState() = default;
  PipelineState(const PipelineState &Other);

  std::unique_ptr<AbstractState> clone() const override;
  bool equals(const AbstractState *Other) const override;
  bool join(const AbstractState *Other) override;
  std::string toString() const override;
};

} // namespace llvm

#endif // ANALYSIS_PIPELINE_H
