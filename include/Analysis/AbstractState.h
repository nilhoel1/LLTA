#ifndef ABSTRACT_STATE_H
#define ABSTRACT_STATE_H

#include <memory>
#include <string>

namespace llvm {

/**
 * Interface for an Abstract State.
 * Represents a state in the abstract domain (e.g., pipeline state, cache
 * state).
 */
class AbstractState {
public:
  virtual ~AbstractState() = default;

  /**
   * Clone the state.
   */
  virtual std::unique_ptr<AbstractState> clone() const = 0;

  /**
   * Check if this state is equal to another state.
   */
  virtual bool equals(const AbstractState *Other) const = 0;

  /**
   * Join this state with another state.
   * Returns true if the state changed.
   */
  virtual bool join(const AbstractState *Other) = 0;

  /**
   * Get a string representation of the state for debugging/graphing.
   */
  virtual std::string toString() const = 0;
};

} // namespace llvm

#endif // ABSTRACT_STATE_H
