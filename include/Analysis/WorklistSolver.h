#ifndef WORKLIST_SOLVER_H
#define WORKLIST_SOLVER_H

#include "AbstractAnalysable.h"
#include "AbstractStateGraph.h"
#include "RTTargets/ProgramGraph.h"
#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include <deque>
#include <map>
#include <set>

namespace llvm {

class TimingAnalysisResults;
class MachineFunction;
class MachineLoopInfo;
class MachineBasicBlock;

/**
 * Worker class for executing an Abstract Analysis.
 * Implements a worklist algorithm to compute the fixpoint.
 */
class WorklistSolver {
public:
  WorklistSolver(AbstractAnalysable &Analysis, AbstractStateGraph &Graph)
      : Analysis(Analysis), Graph(Graph) {}

  /**
   * Run the analysis on all functions in the module.
   */
  void runOnModule(MachineModuleInfo &MMI);

  const AbstractStateGraph &getGraph() const { return Graph; }

  /**
   * Run the analysis on the given function.
   */
  void run(const ProgramGraph &PG);
  void run(MachineFunction &MF, MachineLoopInfo *MLI = nullptr,
           const std::map<const MachineBasicBlock *, unsigned> *LoopBounds =
               nullptr);

private:
  AbstractAnalysable &Analysis;
  AbstractStateGraph &Graph;
  std::deque<unsigned> Worklist;
  std::set<unsigned> InWorklist;

  void addToWorklist(unsigned NodeId);
  unsigned takeFromWorklist();
  void initializeGraph(const ProgramGraph &PG);
  void initializeGraph(
      MachineFunction &MF, MachineLoopInfo *MLI,
      const std::map<const MachineBasicBlock *, unsigned> *LoopBounds);
};

} // namespace llvm

#endif // WORKLIST_SOLVER_H
