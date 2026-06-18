#include "ILP/ILPSolver.h"

namespace llvm {

ILPSolverType parseILPSolverType(const std::string &SolverName) {
  if (SolverName == "gurobi")
    return ILPSolverType::Gurobi;
  if (SolverName == "highs")
    return ILPSolverType::HiGHS;
  return ILPSolverType::Auto;
}

} // namespace llvm
