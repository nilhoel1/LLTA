#include "ILP/ILPSolver.h"
#include "llvm/Support/raw_ostream.h"

#ifdef ENABLE_GUROBI
#include "ILP/GurobiSolver.h"
#endif

#ifdef ENABLE_HIGHS
#include "ILP/HighsSolver.h"
#endif

namespace llvm {

ILPSolverType parseILPSolverType(const std::string &SolverName) {
  if (SolverName == "gurobi") {
    return ILPSolverType::Gurobi;
  } else if (SolverName == "highs") {
    return ILPSolverType::HiGHS;
  }
  return ILPSolverType::Auto;
}

std::unique_ptr<ILPSolver> createILPSolver(ILPSolverType Type) {
  switch (Type) {
  case ILPSolverType::Gurobi:
#ifdef ENABLE_GUROBI
  {
    auto Solver = std::make_unique<GurobiSolver>();
    if (Solver->isAvailable()) {
      return Solver;
    }
    outs() << "Gurobi requested but not available (no license or not built)\n";
    return nullptr;
  }
#else
    outs() << "Gurobi support not compiled in\n";
    return nullptr;
#endif

  case ILPSolverType::HiGHS:
#ifdef ENABLE_HIGHS
  {
    auto Solver = std::make_unique<HighsSolver>();
    if (Solver->isAvailable()) {
      return Solver;
    }
    outs() << "HiGHS requested but not available\n";
    return nullptr;
  }
#else
    outs() << "HiGHS support not compiled in\n";
    return nullptr;
#endif

  case ILPSolverType::Auto:
    // Try Gurobi first, then fall back to HiGHS
#ifdef ENABLE_GUROBI
  {
    auto GurobiSolverPtr = std::make_unique<GurobiSolver>();
    if (GurobiSolverPtr->isAvailable()) {
      outs() << "Auto-selected Gurobi solver\n";
      return GurobiSolverPtr;
    }
    outs() << "Gurobi not available, trying HiGHS...\n";
  }
#endif

#ifdef ENABLE_HIGHS
  {
    auto HighsSolverPtr = std::make_unique<HighsSolver>();
    if (HighsSolverPtr->isAvailable()) {
      outs() << "Auto-selected HiGHS solver\n";
      return HighsSolverPtr;
    }
  }
#endif

    outs() << "No ILP solver available\n";
    return nullptr;
  }
}

} // namespace llvm
