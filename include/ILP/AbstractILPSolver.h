#ifndef ABSTRACT_ILP_SOLVER_H
#define ABSTRACT_ILP_SOLVER_H

#include "Analysis/AbstractStateGraph.h"
#include <map>
#include <string>
#include <vector>

namespace llvm {

struct AbstractILPResult {
  double WCET;
  std::vector<unsigned>
      WorstCasePath; // Sequence of Node IDs (AbstractStateGraph Node IDs)
  std::map<unsigned, double> ExecutionCounts;
  // Solver model status when no optimal solution was produced (e.g.
  // "Infeasible", "Unbounded"). Empty on success. Surfaced by PathAnalysisPass
  // to make a failed solve diagnosable instead of a silent WCET <= 0.
  std::string Status;
};

class AbstractILPSolver {
public:
  virtual ~AbstractILPSolver() = default;

  /**
   * Solves the WCET problem on the given AbstractStateGraph.
   * returning the WCET and the path.
   */
  virtual AbstractILPResult solveWCET(const AbstractStateGraph &ASG) = 0;
};

} // namespace llvm

#endif // ABSTRACT_ILP_SOLVER_H
