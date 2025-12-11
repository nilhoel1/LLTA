//===-- lib/Analysis/SystemState.cpp - SystemState Implementation -*- C++
//-*-===//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the SystemState class, representing the hardware state
/// at a program point during WCET analysis.
///
//===----------------------------------------------------------------------===//

#include "Analysis/SystemState.h"

#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;

namespace llta {

bool SystemState::equals(const AbstractState &Other) const {
  const auto *OtherState = dynamic_cast<const SystemState *>(&Other);
  if (!OtherState)
    return false;
  return *this == *OtherState;
}

void SystemState::print(raw_ostream &OS) const {
  OS << "SystemState { CycleCount: " << CycleCount;
  OS << ", Resources: {";
  bool First = true;
  for (const auto &Pair : ResourceAvailability) {
    if (!First)
      OS << ", ";
    OS << Pair.first << ": " << Pair.second;
    First = false;
  }
  OS << "} }";
}

std::unique_ptr<AbstractState> SystemState::clone() const {
  auto NewState = std::make_unique<SystemState>();
  NewState->CycleCount = CycleCount;
  NewState->ResourceAvailability = ResourceAvailability;
  return NewState;
}

void SystemState::join(const SystemState &Other) {
  // For WCET analysis, we take the maximum (conservative upper bound)
  CycleCount = std::max(CycleCount, Other.CycleCount);

  // For resource availability, take the max (latest availability time)
  for (const auto &Pair : Other.ResourceAvailability) {
    auto It = ResourceAvailability.find(Pair.first);
    if (It == ResourceAvailability.end()) {
      ResourceAvailability[Pair.first] = Pair.second;
    } else {
      It->second = std::max(It->second, Pair.second);
    }
  }
}

void SystemState::advanceClock(uint64_t Cycles) { CycleCount += Cycles; }

} // namespace llta
