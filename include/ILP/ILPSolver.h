#ifndef ILP_SOLVER_H
#define ILP_SOLVER_H

#include <string>

namespace llvm {

/// Selects which abstract ILP solver backend to use for WCET computation.
enum class ILPSolverType { Auto, Gurobi, HiGHS };

/// Parse the backend from the -ilp-solver option string ("auto", "gurobi",
/// "highs"); anything else maps to Auto.
ILPSolverType parseILPSolverType(const std::string &SolverName);

} // namespace llvm

#endif // ILP_SOLVER_H
