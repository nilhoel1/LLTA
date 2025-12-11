#ifndef LLTA_ANALYSIS_SYSTEMSTATE_H
#define LLTA_ANALYSIS_SYSTEMSTATE_H

#include "AbstractState.h"
#include <map>
#include <memory>

namespace llta {

/// Represents the instantaneous snapshot of the hardware.
/// Inherits from AbstractState to integrate with the WorklistSolver.
class SystemState : public AbstractState {
public:
  /// The current Worst-Case Execution Time in cycles up to this point.
  uint64_t CycleCount = 0;

  /// Tracks when specific hardware resources (ALUs, Ports) become free.
  /// Key: ResourceID (from LLVM MCSchedModel), Value: Cycle available.
  std::map<unsigned, uint64_t> ResourceAvailability;

  /// Note: Real implementation might include CacheState/HistoryRegister here.
  /// Ideally, use a PImpl idiom or pointer to a State structure to keep this
  /// lightweight.

  // AbstractState interface implementation
  bool equals(const AbstractState &Other) const override;
  void print(llvm::raw_ostream &OS) const override;
  std::unique_ptr<AbstractState> clone() const override;

  bool operator==(const SystemState &Other) const {
    return CycleCount == Other.CycleCount &&
           ResourceAvailability == Other.ResourceAvailability;
  }

  bool operator!=(const SystemState &Other) const { return !(*this == Other); }

  /// The Join operator (Lattice Union).
  /// Merges state from predecessor blocks (e.g., takes max CycleCount).
  void join(const SystemState &Other);

  /// Helper to advance time (pipeline stall or instruction execution).
  void advanceClock(uint64_t Cycles);
};

} // namespace llta

#endif
