#ifndef GUROBI_SOLVER_H
#define GUROBI_SOLVER_H

#include "ILP/ILPSolver.h"

namespace llvm {

/// Gurobi-based ILP solver implementation
class GurobiSolver : public ILPSolver {
public:
  GurobiSolver();
  ~GurobiSolver() override;

  ILPResult
  solveWCET(const ProgramGraph &MASG, unsigned EntryNodeId, unsigned ExitNodeId,
            const std::map<unsigned, unsigned> &LoopBoundMap) override;

  std::string getName() const override { return "Gurobi"; }

  bool isAvailable() const override;

private:
  bool HasLicense;
};

} // namespace llvm

#endif // GUROBI_SOLVER_H
