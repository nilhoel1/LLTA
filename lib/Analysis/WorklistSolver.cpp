//===-- lib/Analysis/WorklistSolver.cpp - Worklist Solver Impl ---*- C++
//-*-===//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the WorklistSolver class, which drives the dataflow
/// analysis to a fixpoint using a standard worklist algorithm.
///
//===----------------------------------------------------------------------===//

#include "WorklistSolver.h"

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/Debug.h"

#include <deque>

#define DEBUG_TYPE "llta-worklist-solver"

using namespace llvm;

namespace llta {

WorklistSolver::WorklistSolver(const MachineFunction &MF,
                               AbstractAnalysis &Analysis)
    : Analysis(Analysis), MF(MF) {}

void WorklistSolver::solve() {
  // Initialize the worklist with all basic blocks
  std::deque<const MachineBasicBlock *> Worklist;

  // Get the entry block and initialize its state
  const MachineBasicBlock *EntryBB = &MF.front();
  BlockStates[EntryBB] = Analysis.getInitialState();

  // Add entry block to the worklist
  Worklist.push_back(EntryBB);

  // Process the worklist until empty (fixpoint reached)
  while (!Worklist.empty()) {
    const MachineBasicBlock *CurrentBB = Worklist.front();
    Worklist.pop_front();

    // Get the current state for this block (or create bottom state)
    std::unique_ptr<AbstractState> CurrentState;

    // Join states from all predecessors
    bool HasPredecessors = false;
    for (const MachineBasicBlock *Pred : CurrentBB->predecessors()) {
      auto It = BlockStates.find(Pred);
      if (It != BlockStates.end() && It->second) {
        if (!HasPredecessors) {
          CurrentState = It->second->clone();
          HasPredecessors = true;
        } else {
          CurrentState = Analysis.join(*CurrentState, *It->second);
        }
      }
    }

    // If no predecessors with state and this is entry block, use initial state
    if (!HasPredecessors) {
      if (CurrentBB == EntryBB) {
        CurrentState = Analysis.getInitialState();
      } else {
        // Block has no reachable predecessors yet, skip for now
        continue;
      }
    }

    // Apply transfer function to each instruction in the block
    for (const MachineInstr &MI : *CurrentBB) {
      CurrentState = Analysis.transfer(*CurrentState, MI);
    }

    // Check if the state has changed for successors
    for (const MachineBasicBlock *Succ : CurrentBB->successors()) {
      auto It = BlockStates.find(Succ);

      if (It == BlockStates.end() || !It->second) {
        // First time reaching this successor, initialize with current state
        BlockStates[Succ] = CurrentState->clone();
        Worklist.push_back(Succ);
      } else {
        // Join with existing state
        auto JoinedState = Analysis.join(*It->second, *CurrentState);

        // Check if state changed using isLessOrEqual
        // If Old <= New and New <= Old, they are equal (no change)
        bool OldLeqNew = Analysis.isLessOrEqual(*It->second, *JoinedState);
        bool NewLeqOld = Analysis.isLessOrEqual(*JoinedState, *It->second);

        if (!OldLeqNew || !NewLeqOld) {
          // State has changed, update and add to worklist
          BlockStates[Succ] = std::move(JoinedState);
          Worklist.push_back(Succ);
        }
      }
    }
  }

  LLVM_DEBUG(dbgs() << "WorklistSolver: Fixpoint reached for function "
                    << MF.getName() << "\n");
}

} // namespace llta
