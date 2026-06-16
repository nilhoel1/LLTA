#ifndef ABSTRACT_GUROBI_SOLVER_H
#define ABSTRACT_GUROBI_SOLVER_H

#include "AbstractILPSolver.h"

namespace llvm {

class AbstractGurobiSolver : public AbstractILPSolver {
public:
  AbstractGurobiSolver();
  ~AbstractGurobiSolver() override;

  AbstractILPResult solveWCET(const AbstractStateGraph &ASG) override;
};

} // namespace llvm

#endif // ABSTRACT_GUROBI_SOLVER_H
