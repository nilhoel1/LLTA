#ifndef ABSTRACT_ANALYSABLE_H
#define ABSTRACT_ANALYSABLE_H

#include "AbstractState.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <memory>

namespace llvm {

/**
 * Interface for an analysis component (e.g., Pipeline Analysis, Cache
 * Analysis). Defines the initial state and transfer functions.
 */
class AbstractAnalysable {
public:
  virtual ~AbstractAnalysable() = default;

  /**
   * Create the initial abstract state for the analysis.
   */
  virtual std::unique_ptr<AbstractState> getInitialState() = 0;

  /**
   * Apply the transfer function for a machine instruction.
   * Modifies the state in-place and returns the cycle cost.
   */
  virtual unsigned process(AbstractState *State, const MachineInstr *MI) = 0;
};

} // namespace llvm

#endif // ABSTRACT_ANALYSABLE_H
