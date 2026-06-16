#include "Analysis/AbstractStateGraph.h"
#include "ILP/AbstractILPSolver.h"

namespace llvm {

class AbstractGurobiSolver : AbstractILPSolver{
  AbstractGurobiSolver();

  ~AbstractGurobiSolver();

  AbstractILPResult solveWCET(const AbstractStateGraph &ASG);
};
} // namespace llvm