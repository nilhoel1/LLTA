//===-- Analysis/WorklistSolver.h - Generic Worklist Solver -*- C++ -*-===//
//
// Part of the LLTA Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the WorklistSolver class, which implements a standard
/// worklist algorithm to drive the dataflow analysis to a fixpoint. It is
/// fully agnostic to the specific analysis domain, relying on the
/// AbstractAnalysis interface.
///
//===----------------------------------------------------------------------===//

#ifndef LLTA_ANALYSIS_WORKLISTSOLVER_H
#define LLTA_ANALYSIS_WORKLISTSOLVER_H

#include "AbstractAnalysis.h"
#include "AbstractState.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <map>
#include <memory>

namespace llta {

/// \brief A generic solver engine implementing the worklist algorithm.
///
/// This solver manages the flow of information through the Control Flow Graph
/// (CFG) of a MachineFunction. It maintains the mapping between Basic Blocks
/// and their corresponding abstract states, and iteratively updates them until
/// convergence (fixpoint) is reached.
class WorklistSolver {
  /// Reference to the underlying analysis logic (Transfer functions, Join).
  AbstractAnalysis &Analysis;

  /// The MachineFunction being analyzed.
  const llvm::MachineFunction &MF;

  /// Map storing the IN-state for each basic block.
  std::map<const llvm::MachineBasicBlock *, std::unique_ptr<AbstractState>>
      BlockStates;

public:
  /// \brief Constructor initializing the solver with the function and analysis.
  /// \param MF The LLVM MachineFunction to analyze.
  /// \param Analysis Reference to the concrete analysis implementation.
  WorklistSolver(const llvm::MachineFunction &MF, AbstractAnalysis &Analysis);

  /// \brief Executes the worklist algorithm to compute the fixpoint.
  void solve();
};

} // namespace llta

#endif // LLTA_ANALYSIS_WORKLISTSOLVER_H
