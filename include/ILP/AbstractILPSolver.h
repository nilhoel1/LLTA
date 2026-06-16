#ifndef ABSTRACT_ILP_SOLVER_H
#define ABSTRACT_ILP_SOLVER_H

#include "Analysis/AbstractStateGraph.h"
#include <map>
#include <vector>

namespace llvm {

struct AbstractILPResult {
  double WCET;
  std::vector<unsigned>
      WorstCasePath; // Sequence of Node IDs (AbstractStateGraph Node IDs)
  std::map<unsigned, double> ExecutionCounts;
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
