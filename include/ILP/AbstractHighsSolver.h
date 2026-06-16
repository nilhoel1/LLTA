#ifndef ABSTRACT_HIGHS_SOLVER_H
#define ABSTRACT_HIGHS_SOLVER_H

#include "AbstractILPSolver.h"

namespace llvm {

class AbstractHighsSolver : public AbstractILPSolver {
public:
  AbstractHighsSolver();
  ~AbstractHighsSolver() override;

  AbstractILPResult solveWCET(const AbstractStateGraph &ASG) override;
};

} // namespace llvm

#endif // ABSTRACT_HIGHS_SOLVER_H
