//===-- Analysis/AbstractState.h - Abstract Analysis State -*- C++ -*-===//
//
// Part of the LLTA Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the AbstractState interface, which serves as the abstract
/// base class for any lattice element/state used in the WCET analysis.
///
//===----------------------------------------------------------------------===//

#ifndef LLTA_ANALYSIS_ABSTRACTSTATE_H
#define LLTA_ANALYSIS_ABSTRACTSTATE_H

#include "llvm/Support/raw_ostream.h"
#include <memory>

namespace llta {

/// \brief Abstract base class representing the state of the system at a program
/// point.
///
/// This class defines the interface that any domain-specific state (e.g., WCET
/// IPET state, Abstract Interpretation state) must implement. It supports
/// polymorphic operations like deep copying (cloning) and equality checking
/// to be used by the generic solver.
class AbstractState {
public:
  virtual ~AbstractState() = default;

  /// \brief Checks if this state is semantically equal to another state.
  /// \param Other The state to compare against.
  /// \return True if the states are equal, false otherwise.
  virtual bool equals(const AbstractState &Other) const = 0;

  /// \brief Prints the state content to the given output stream.
  /// Used for debugging and generating reports.
  virtual void print(llvm::raw_ostream &OS) const = 0;

  /// \brief Creates a deep copy of the current state.
  /// \return A unique_ptr containing the cloned state.
  virtual std::unique_ptr<AbstractState> clone() const = 0;
};

} // namespace llta

#endif // LLTA_ANALYSIS_ABSTRACTSTATE_H
