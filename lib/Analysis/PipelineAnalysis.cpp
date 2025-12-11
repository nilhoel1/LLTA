#include "Analysis/PipelineAnalysis.h"
#include "Analysis/Targets/MSP430Latency.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace llta {

PipelineAnalysis::PipelineAnalysis(const TargetSubtargetInfo &STI) {
  SchedModel.init(&STI);
  // Initialize strategies (defaults for now)
  // ICache = std::make_unique<LRUCache>(...);
}

// AbstractAnalysis interface implementation

std::unique_ptr<AbstractState> PipelineAnalysis::getInitialState() {
  auto State = std::make_unique<SystemState>();
  State->CycleCount = 0;
  return State;
}

std::unique_ptr<AbstractState>
PipelineAnalysis::transfer(const AbstractState &FromState,
                           const MachineInstr &MI) {
  const auto *SysState = dynamic_cast<const SystemState *>(&FromState);
  if (!SysState) {
    llvm_unreachable("PipelineAnalysis::transfer: Invalid state type");
  }

  SystemState OutState = *SysState;

  // 1. Static Physics
  unsigned BaseLatency = computeBaseLatency(MI, OutState);

  // 2. Dynamic Penalties
  unsigned Penalties = computeDynamicPenalties(MI);

  // Update State
  OutState.advanceClock(BaseLatency + Penalties);

  return std::make_unique<SystemState>(OutState);
}

std::unique_ptr<AbstractState> PipelineAnalysis::join(const AbstractState &S1,
                                                      const AbstractState &S2) {
  const auto *State1 = dynamic_cast<const SystemState *>(&S1);
  const auto *State2 = dynamic_cast<const SystemState *>(&S2);

  if (!State1 || !State2) {
    llvm_unreachable("PipelineAnalysis::join: Invalid state types");
  }

  auto Result = std::make_unique<SystemState>(*State1);
  Result->join(*State2);
  return Result;
}

bool PipelineAnalysis::isLessOrEqual(const AbstractState &S1,
                                     const AbstractState &S2) const {
  const auto *State1 = dynamic_cast<const SystemState *>(&S1);
  const auto *State2 = dynamic_cast<const SystemState *>(&S2);

  if (!State1 || !State2) {
    llvm_unreachable("PipelineAnalysis::isLessOrEqual: Invalid state types");
  }

  // For WCET analysis with max-based join:
  // S1 <= S2 iff S1.CycleCount <= S2.CycleCount and all resources in S1 <= S2
  if (State1->CycleCount > State2->CycleCount)
    return false;

  for (const auto &Pair : State1->ResourceAvailability) {
    auto It = State2->ResourceAvailability.find(Pair.first);
    if (It == State2->ResourceAvailability.end()) {
      // S2 doesn't have this resource, so S1 has more info
      return false;
    }
    if (Pair.second > It->second) {
      return false;
    }
  }

  return true;
}

// Legacy interface for backward compatibility

SystemState PipelineAnalysis::getEntryState() const {
  SystemState State;
  State.CycleCount = 0;
  return State;
}

SystemState PipelineAnalysis::getBottomState() const {
  // Initialize with conceptually infinite time or error state
  SystemState State;
  State.CycleCount = 0;
  return State;
}

SystemState PipelineAnalysis::transferState(const MachineInstr &MI,
                                            SystemState InState) {
  SystemState OutState = InState;

  // 1. Static Physics
  unsigned BaseLatency = computeBaseLatency(MI, OutState);

  // 2. Dynamic Penalties
  unsigned Penalties = computeDynamicPenalties(MI);

  // Update State
  OutState.advanceClock(BaseLatency + Penalties);

  return OutState;
}

unsigned PipelineAnalysis::computeBaseLatency(const MachineInstr &MI,
                                              SystemState &State) {
  // Check for MSP430 Architecture
  //  Note: We need a way to check the architecture.
  //  In a real pass we might store the Triple key or Subtarget.
  //  For now, let's assume if the SchedModel claims to be MSP430 or we can
  //  check the opcode range? Actually, standard way is checking the Triple.
  //  Since we don't have the Triple stored easily here without passing it in
  //  Constructor, we might adding it to the class members. BUT, for this
  //  specific request "marry it", I will trust the user context or add a check
  //  if possible. Let's rely on the Opcodes namespace or just include the check
  //  if we had the STI.

  // However, the `getMSP430Latency` function handles MSP430 opcodes.
  // If we pass a non-MSP430 instruction, it might not work or return 0.
  // Let's try to detect it via the MI.getMF()->getTarget()... chain if
  // available.
  const auto &MF = *MI.getMF();
  const auto &Target = MF.getTarget();
  if (Target.getTargetTriple().getArch() == Triple::msp430) {
    return targets::getMSP430Latency(MI);
  }

  // Fallback to Standard LLVM SchedModel
  if (!SchedModel.hasInstrSchedModel())
    return 1; // Default safety

  // Calculate generic latency (simplified for now)
  unsigned Latency = SchedModel.computeInstrLatency(&MI);

  // TODO: Check resource availability in State.ResourceAvailability

  return Latency;
}

unsigned PipelineAnalysis::computeDynamicPenalties(const MachineInstr &MI) {
  unsigned Penalty = 0;
  // TODO: Query ICache and BPredictor
  return Penalty;
}

} // namespace llta
