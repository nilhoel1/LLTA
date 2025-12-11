//===-- Analysis/AbstractAnalysis.h - Abstract Analysis Logic -*- C++ -*-===//
//
// Part of the LLTA Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the AbstractAnalysis interface, which encapsulates the
/// transfer functions and lattice operations (join, meet) for the analysis.
///
//===----------------------------------------------------------------------===//

#ifndef LLTA_ANALYSIS_ABSTRACTANALYSIS_H
#define LLTA_ANALYSIS_ABSTRACTANALYSIS_H

#include "AbstractState.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <memory>

namespace llta {

/// \brief Abstract interface for defining the static analysis logic.
///
/// Concrete implementations (e.g., PipelineAnalysis) must inherit from this
/// class and implement the transfer functions and lattice operators. The solver
/// uses this interface to propagate states through the CFG without knowing
/// the specifics of the underlying analysis domain.
class AbstractAnalysis {
public:
  virtual ~AbstractAnalysis() = default;

  /// \brief Generally the "Transfer Function" in dataflow analysis.
  /// Computes the effect of executing a machine instruction on an input state.
  ///
  /// \param FromState The state of the system before executing the instruction.
  /// \param MI The machine instruction being executed.
  /// \return A new unique_ptr containing the state after execution.
  virtual std::unique_ptr<AbstractState>
  transfer(const AbstractState &FromState, const llvm::MachineInstr &MI) = 0;

  /// \brief The "Join" operator (Lattice Union/Merge).
  /// Merges two states into a single state. This is typically used at control
  /// flow merge points (e.g., end of if-then-else blocks).
  ///
  /// \param S1 The first state to merge.
  /// \param S2 The second state to merge.
  /// \return A new unique_ptr containing the merged state (Upper Bound).
  virtual std::unique_ptr<AbstractState> join(const AbstractState &S1,
                                              const AbstractState &S2) = 0;

  /// \brief Returns the initial state for the analysis entry point.
  /// Typically corresponds to the state at the beginning of the function.
  virtual std::unique_ptr<AbstractState> getInitialState() = 0;

  /// \brief Checks the partial order relation (S1 <= S2).
  /// Used to determine if the fixpoint iteration has converged.
  ///
  /// \param S1 The first state (LHS).
  /// \param S2 The second state (RHS).
  /// \return True if S1 is less than or equal to S2 in the lattice.
  virtual bool isLessOrEqual(const AbstractState &S1,
                             const AbstractState &S2) const = 0;
};

} // namespace llta

#endif // LLTA_ANALYSIS_ABSTRACTANALYSIS_H
