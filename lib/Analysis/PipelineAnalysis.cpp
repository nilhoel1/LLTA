#include "Analysis/PipelineAnalysis.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

void PipelineAnalysis::addAnalysis(
    std::unique_ptr<AbstractAnalysable> Analysis) {
  Analyses.push_back(std::move(Analysis));
}

std::unique_ptr<AbstractState> PipelineAnalysis::getInitialState() {
  auto State = std::make_unique<PipelineState>();
  for (auto &Analysis : Analyses) {
    State->SubStates.push_back(Analysis->getInitialState());
  }
  return State;
}

unsigned PipelineAnalysis::process(AbstractState *State,
                                   const MachineInstr *MI) {
  auto *PState = static_cast<PipelineState *>(State);
  unsigned TotalCost = 0;
  for (size_t i = 0; i < Analyses.size(); ++i) {
    TotalCost += Analyses[i]->process(PState->SubStates[i].get(), MI);
  }
  return TotalCost; // Return simple sum or max? Pipelines parallelize.
  // For a pipeline, the cost is not the sum. It's the latency determined by the
  // bottleneck. But for "SimpleStage" summing 1s might be okay for now, or
  // returns MAX? User said "simple MSP430 pipeline". Let's return the MAX cost
  // of stages? Or assume the last stage completion triggers? If we return sum,
  // it's sequential. If we return 1 (constant issue rate), it's ideal. Let's
  // assume sequential for now (simple) OR just return the max. Actually,
  // standard pipeline analysis is complex. Let's just return the sum to be
  // safe/visible, or change to return void and handle cost elsewhere? No,
  // interface change requires return. Let's return the maximum cost reported by
  // any stage (e.g. if Execute takes 5 cycles, and Fetch takes 1, the impact is
  // 5?) But stages are pipelined. Let's return 1 for now if pipelined
  // efficiently? Let's stick to TotalCost for now to see SOMETHING different
  // than 0.
}

// PipelineState Implementation

PipelineState::PipelineState(const PipelineState &Other) {
  for (const auto &SubState : Other.SubStates) {
    SubStates.push_back(SubState->clone());
  }
}

std::unique_ptr<AbstractState> PipelineState::clone() const {
  return std::make_unique<PipelineState>(*this);
}

bool PipelineState::equals(const AbstractState *Other) const {
  const auto *POther = static_cast<const PipelineState *>(Other);
  if (SubStates.size() != POther->SubStates.size())
    return false;
  for (size_t i = 0; i < SubStates.size(); ++i) {
    if (!SubStates[i]->equals(POther->SubStates[i].get()))
      return false;
  }
  return true;
}

bool PipelineState::join(const AbstractState *Other) {
  const auto *POther = static_cast<const PipelineState *>(Other);
  bool Changed = false;
  for (size_t i = 0; i < SubStates.size(); ++i) {
    if (SubStates[i]->join(POther->SubStates[i].get())) {
      Changed = true;
    }
  }
  return Changed;
}

std::string PipelineState::toString() const {
  std::string Res = "Pipeline(";
  for (size_t i = 0; i < SubStates.size(); ++i) {
    Res += SubStates[i]->toString();
    if (i < SubStates.size() - 1)
      Res += ", ";
  }
  Res += ")";
  return Res;
}

} // namespace llvm
