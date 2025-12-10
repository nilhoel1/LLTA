#ifndef LLTA_ANALYSIS_HARDWARESTRATEGIES_H
#define LLTA_ANALYSIS_HARDWARESTRATEGIES_H

#include "llvm/CodeGen/MachineInstr.h"
#include <cstdint>

namespace llta {

/// Abstract base class for Branch Prediction logic.
class BranchPredictorStrategy {
public:
  virtual ~BranchPredictorStrategy() = default;

  enum PredictionResult { Correct, Mispredicted, Unknown };

  /// Returns the prediction result for a specific branch instruction
  /// given the current global history (if modeled).
  virtual PredictionResult predict(const llvm::MachineInstr &MI) = 0;

  /// Updates the internal state of the predictor (e.g., History Register).
  virtual void update(const llvm::MachineInstr &MI, bool Taken) = 0;
};

/// Abstract base class for Cache hierarchies.
class CacheStrategy {
public:
  virtual ~CacheStrategy() = default;

  enum AccessResult { Hit, Miss, Unknown };

  /// Models a memory access to a specific abstract address.
  /// Returns Hit/Miss to calculate latency penalties.
  virtual AccessResult access(uint64_t AbstractAddress, bool IsWrite) = 0;
};

} // namespace llta

#endif // LLTA_ANALYSIS_HARDWARESTRATEGIES_H