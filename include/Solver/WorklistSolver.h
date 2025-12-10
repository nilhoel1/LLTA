#ifndef LLTA_SOLVER_WORKLISTSOLVER_H
#define LLTA_SOLVER_WORKLISTSOLVER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"

namespace llta {

template <typename AnalysisDomain> class WorklistSolver {
  AnalysisDomain &Domain;
  using StateType = typename AnalysisDomain::StateType; // Expected from Domain

  llvm::DenseMap<const llvm::MachineBasicBlock *, StateType> BlockStates;
  llvm::SmallVector<llvm::MachineBasicBlock *, 32> Worklist;

public:
  WorklistSolver(AnalysisDomain &D) : Domain(D) {}

  void solve(llvm::MachineFunction &MF);
};

// Implementation included in header for templates
template <typename AnalysisDomain>
void WorklistSolver<AnalysisDomain>::solve(llvm::MachineFunction &MF) {
  // ... (Standard worklist algorithm as discussed previously) ...
}

} // namespace llta

#endif
