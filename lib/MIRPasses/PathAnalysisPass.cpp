#include "MIRPasses/PathAnalysisPass.h"
#include "ILP/AbstractHighsSolver.h"
#include "ILP/AbstractILPSolver.h"
#include "MIRPasses/StartFunction.h"
#include "Targets/RTTarget.h"
#include "TimingAnalysisResults.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <climits>
#include <cmath>
#include <memory>
#include <string>

namespace llvm {

char PathAnalysisPass::ID = 0;

/**
 * @brief Construct a new PathAnalysisPass object
 *
 * @param TAR TimingAnalysisResults reference
 */
PathAnalysisPass::PathAnalysisPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR),
      AnalysisWorker(TAR.getTarget().getPipeline(), ASG) {}

Function *PathAnalysisPass::getStartingFunction(CallGraph &CG) {
  Function *StartingFunction = findStartFunction(CG);
  if (DebugPrints && StartingFunction)
    outs() << "StartingFunction: " << StartingFunction->getName() << "\n";
  return StartingFunction;
}

/**
 * @brief Finalize the path analysis by solving the WCET ILP.
 *
 * Runs abstract interpretation over the pre-built ProgramGraph (MASG) to build
 * the AbstractStateGraph, then solves the WCET maximization ILP on it with the
 * HiGHS backend.
 */
bool PathAnalysisPass::doFinalization(Module &M) {
  outs() << "\n=== Path Analysis: Computing WCET via ILP ===\n";

  const auto &Nodes = TAR.MASG.getNodes();
  outs() << "Total nodes: " << Nodes.size() << "\n";

  if (DebugPrints) {
    for (const auto &NodePair : Nodes) {
      const Node &N = NodePair.second;
      if (N.IsLoop) {
        outs() << "Loop node " << NodePair.first << " (" << N.Name
               << ") has bound: " << N.UpperLoopBound;
        if (N.IsNestedLoop && N.NestedLoopHeader)
          outs() << " (nested in loop " << N.NestedLoopHeader->Id << ")";
        outs() << "\n";
      }
    }
  }

  // Build the AbstractStateGraph (abstract interpretation over MASG), then
  // solve the WCET ILP on it.
  AnalysisWorker.run(TAR.MASG);

  // Solve the WCET ILP with the HiGHS backend.
  std::unique_ptr<AbstractILPSolver> Solver;
  std::string SolverName;

#ifdef ENABLE_HIGHS
  Solver = std::make_unique<AbstractHighsSolver>();
  SolverName = "HiGHS";
#endif

  if (!Solver) {
    outs() << "Error: No ILP solver available. Cannot compute WCET.\n";
    return false;
  }

  outs() << "Using ILP solver: " << SolverName << "\n";
  outs() << "\nSolving WCET ILP...\n";
  auto Result = Solver->solveWCET(AnalysisWorker.getGraph());

  outs() << "\n=== WCET Analysis Results ===\n";
  if (Result.WCET > 0) {
    // The ILP objective is integral in theory; the solver returns a double that
    // may carry tiny floating-point noise (e.g. 6346.9999). Round to the
    // nearest integer to recover the true cycle count (truncating would
    // under-report the WCET, which is unsound).
    outs() << "WCET (worst-case execution time): "
           << static_cast<unsigned>(std::llround(Result.WCET)) << " cycles\n";

    // Surface any reason the bound may be an under-approximation rather than a
    // valid upper bound. The numeric WCET line above is kept verbatim (the
    // regression harness greps for it); this block is purely additive.
    const auto &Reasons = TAR.getUnsoundReasons();
    const auto &Callees = TAR.MASG.getUnsoundExternalCallees();
    if (!Reasons.empty() || !Callees.empty()) {
      outs()
          << "\n*** WARNING: this WCET is UNSOUND (under-approximation) ***\n";
      for (const auto &R : Reasons)
        outs() << "  - " << R << "\n";
      if (!Callees.empty()) {
        outs() << "  - reachable calls into body-less functions were costed as "
                  "0 cycles (no cost model):\n";
        for (const auto &C : Callees)
          outs() << "      " << C << "\n";
      }
    }
  } else {
    // Keep the literal "Failed to compute WCET." prefix (the regression harness
    // keys off the absence of the WCET line); append the solver's model status
    // when known so the failure mode (e.g. Infeasible / Unbounded) is visible.
    outs() << "Failed to compute WCET.";
    if (!Result.Status.empty())
      outs() << " (solver status: " << Result.Status << ")";
    outs() << "\n";
    return false;
  }

  return false;
}

/**
 * @brief Iterates over MachineFunction and performs path analysis
 *
 * @param F MachineFunction to analyze
 * @return true if the function was modified (always false for analysis)
 */
bool PathAnalysisPass::runOnMachineFunction(MachineFunction &F) {
  // We do not run analysis per-function anymore.
  // We wait until doFinalization where the full ProgramGraph (MASG) is ready.
  return false;
}

MachineFunctionPass *createPathAnalysisPass(TimingAnalysisResults &TAR) {
  return new PathAnalysisPass(TAR);
}
} // namespace llvm
