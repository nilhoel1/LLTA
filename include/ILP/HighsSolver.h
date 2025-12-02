#ifndef HIGHS_SOLVER_H
#define HIGHS_SOLVER_H

#include "ILP/ILPSolver.h"

namespace llvm {

/// HiGHS-based ILP solver implementation
class HighsSolver : public ILPSolver {
public:
  HighsSolver();
  ~HighsSolver() override;

  ILPResult
  solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
            unsigned ExitNodeId,
            const std::map<unsigned, unsigned> &LoopBoundMap) override;

  std::string getName() const override { return "HiGHS"; }

  bool isAvailable() const override;
};

} // namespace llvm

#endif // HIGHS_SOLVER_H
