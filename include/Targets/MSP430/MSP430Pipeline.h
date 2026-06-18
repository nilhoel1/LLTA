#ifndef MSP430_PIPELINE_H
#define MSP430_PIPELINE_H

#include "Analysis/PipelineAnalysis.h"

namespace llvm {

/**
 * MSP430 specific pipeline configuration.
 * Using a simple 1-stage pipeline model.
 */
class MSP430Pipeline : public PipelineAnalysis {
public:
  MSP430Pipeline();
};

/**
 * A simple dummy stage analysis for demonstration.
 */
class SimpleStage : public AbstractAnalysable {
public:
  std::string Name;
  SimpleStage(std::string Name) : Name(Name) {}

  std::unique_ptr<AbstractState> getInitialState() override;
  unsigned process(AbstractState *State, const MachineInstr *MI) override;
};

class SimpleStageState : public AbstractState {
public:
  int Val;
  SimpleStageState(int Val = 0) : Val(Val) {}

  std::unique_ptr<AbstractState> clone() const override {
    return std::make_unique<SimpleStageState>(Val);
  }
  bool equals(const AbstractState *Other) const override {
    return Val == static_cast<const SimpleStageState *>(Other)->Val;
  }
  bool join(const AbstractState *Other) override {
    int OtherVal = static_cast<const SimpleStageState *>(Other)->Val;
    if (OtherVal > Val) {
      Val = OtherVal; // Max merge for example
      return true;
    }
    return false;
  }
  std::string toString() const override { return std::to_string(Val); }
};

} // namespace llvm

#endif // MSP430_PIPELINE_H
