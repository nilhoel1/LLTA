#include "Analysis/AbstractStateGraph.h"
#include "ILP/AbstractILPSolver.h"

namespace llvm {

struct AbstractHighsSolver : AbstractILPSolver{
  AbstractHighsSolver();

  ~AbstractHighsSolver();

  AbstractILPResult solveWCET(const AbstractStateGraph &ASG);
};
} // namespace llvm