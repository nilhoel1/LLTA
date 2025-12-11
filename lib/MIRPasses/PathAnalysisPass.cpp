#include "MIRPasses/PathAnalysisPass.h"
#include "ILP/AbstractGurobiSolver.h"
#include "ILP/AbstractHighsSolver.h"
#include "ILP/ILPSolver.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

#ifdef ENABLE_GUROBI
#include "ILP/GurobiSolver.h"
#endif

#ifdef ENABLE_HIGHS
#include "ILP/HighsSolver.h"
#endif

namespace llvm {

char PathAnalysisPass::ID = 0;

/**
 * @brief Construct a new PathAnalysisPass object
 *
 * @param TAR TimingAnalysisResults reference
 */
PathAnalysisPass::PathAnalysisPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR), AnalysisWorker(Pipeline, ASG) {}

Function *PathAnalysisPass::getStartingFunction(CallGraph &CG) {
  // We assume that the Function with the minimal number of References might be
  // the starting Function, e.g. main. If multiple Functions have the same
  // number of references, we can not be sure and return nullptr.
  Function *StartingFunction = nullptr;
  unsigned int CurrentNumReferences = UINT_MAX;
  bool SeenNumRefsTwice = false;

  for (auto &CGNode : CG) {
    auto *F = CGNode.second->getFunction();
    if (F == nullptr)
      continue;
    if (!StartFunctionName.empty() &&
        F->getName().compare(StartFunctionName) == 0) {
      return F;
    }
    auto NumRef = CGNode.second->getNumReferences();
    if (NumRef < CurrentNumReferences) {
      StartingFunction = F;
      CurrentNumReferences = NumRef;
    } else if (NumRef == CurrentNumReferences) {
      SeenNumRefsTwice = true;
    }
  }
  if (SeenNumRefsTwice && StartFunctionName.empty())
    return nullptr;
  if (DebugPrints && StartingFunction)
    outs() << "StartingFunction: " << StartingFunction->getName() << "\n";
  return StartingFunction;
}

/**
 * @brief Finalize the path analysis by solving the WCET ILP
 *
 * This function:
 * 1. Creates an ILP solver based on the command-line option
 * 2. Sets up the WCET maximization problem
 * 3. Solves the ILP and reports results
 *
 * @param MASG The microarchitectural state graph
 * @return true if WCET was successfully computed
 */
bool PathAnalysisPass::doFinalization(Module &M) {
  outs() << "\n=== Path Analysis: Computing WCET via ILP ===\n";

  // Parse solver type from command-line option
  ILPSolverType SolverType = parseILPSolverType(ILPSolverOption);

  // Find entry and exit nodes
  unsigned EntryNodeId = UINT_MAX;
  unsigned ExitNodeId = UINT_MAX;

  const auto &Nodes = TAR.MASG.getNodes();

  for (const auto &NodePair : Nodes) {
    const Node &N = NodePair.second;

    // Check for explicit Entry/Exit nodes
    if (N.Name == "Entry") {
      EntryNodeId = NodePair.first;
    } else if (N.Name == "Exit") {
      ExitNodeId = NodePair.first;
    }
  }

  // Fallback: find nodes with no predecessors/successors
  if (EntryNodeId == UINT_MAX) {
    for (const auto &NodePair : Nodes) {
      if (NodePair.second.getPredecessors().empty()) {
        EntryNodeId = NodePair.first;
        break;
      }
    }
  }

  if (ExitNodeId == UINT_MAX) {
    for (const auto &NodePair : Nodes) {
      if (NodePair.second.getSuccessors().empty()) {
        ExitNodeId = NodePair.first;
        break;
      }
    }
  }

  if (EntryNodeId == UINT_MAX || ExitNodeId == UINT_MAX) {
    outs()
        << "Error: Could not identify entry and/or exit nodes in the graph.\n";
    return false;
  }

  outs() << "Entry node ID: " << EntryNodeId << "\n";
  outs() << "Exit node ID: " << ExitNodeId << "\n";
  outs() << "Total nodes: " << Nodes.size() << "\n";

  // Count edges in the graph
  unsigned NumEdges = 0;
  for (const auto &NodePair : Nodes) {
    NumEdges += NodePair.second.getSuccessors().size();
  }
  outs() << "Total edges: " << NumEdges << "\n";

  // Print loop information (loop bounds are now read directly from
  // Node.UpperLoopBound)
  for (const auto &NodePair : Nodes) {
    const Node &N = NodePair.second;
    if (N.IsLoop) {
      outs() << "Loop node " << NodePair.first << " (" << N.Name
             << ") has bound: " << N.UpperLoopBound;
      if (N.IsNestedLoop && N.NestedLoopHeader) {
        outs() << " (nested in loop " << N.NestedLoopHeader->Id << ")";
      }
      outs() << "\n";
    }
  }

  std::map<unsigned, unsigned> EmptyLoopBoundMap; // Not used anymore

  // Declare Result outside the if/else block so it's accessible later
  ILPResult Result;

  // Handle "all" option - the full comparison table is printed after abstract
  // analysis
  if (SolverType == ILPSolverType::All) {
    // Just mark success for now, unified table will be printed in abstract
    // analysis section
    Result.Success = true;
    Result.ObjectiveValue = 0; // Will be set from unified table
  } else {
    // Single solver mode
    auto Solver = createILPSolver(SolverType);
    if (!Solver) {
      outs() << "Error: No ILP solver available. Cannot compute WCET.\n";
      return false;
    }

    outs() << "Using ILP solver: " << Solver->getName() << "\n";

    // Solve the ILP (loop bounds are read from nodes directly, map not used)
    outs() << "\nSolving WCET ILP...\n";
    Result =
        Solver->solveWCET(TAR.MASG, EntryNodeId, ExitNodeId, EmptyLoopBoundMap);

    // Report results
    outs() << "\n=== WCET Analysis Results ===\n";
    outs() << "Status: " << Result.StatusMessage << "\n";

    if (Result.Success) {
      outs() << "WCET (worst-case execution time): "
             << static_cast<unsigned>(Result.ObjectiveValue) << " cycles\n";

      if (DebugPrints) {
        outs() << "\nNode execution counts:\n";
        for (const auto &[NodeId, Count] : Result.NodeExecutionCounts) {
          if (Count > 0) {
            const Node &N = Nodes.at(NodeId);
            outs() << "  Node " << NodeId << " (" << N.Name
                   << "): " << static_cast<unsigned>(Count) << " times, "
                   << N.getState().getUpperBoundCycles() << " cycles/exec\n";
          }
        }

        if (!Result.EdgeExecutionCounts.empty()) {
          outs() << "\nEdge execution counts:\n";
          for (const auto &[Edge, Count] : Result.EdgeExecutionCounts) {
            if (Count > 0) {
              outs() << "  Edge (" << Edge.first << " -> " << Edge.second
                     << "): " << static_cast<unsigned>(Count) << " times\n";
            }
          }
        }
      }

    } else {
      outs() << "Failed to compute WCET.\n";
      return false;
    }
  }

  // --- Abstract Analysis Verification & Comparison ---
  outs() << "\n=== Abstract Analysis Verification ===\n";

  // Run Abstract Analysis on the pre-built ProgramGraph (MASG)
  // This calculates the AbstractStateGraph nodes/edges/costs
  AnalysisWorker.run(TAR.MASG);

  // Check if we should run all solvers or just one
  if (SolverType == ILPSolverType::All) {
    // Unified result structure for both Legacy and Abstract solvers
    struct UnifiedSolverResult {
      std::string Type;   // "Legacy" or "Abstract"
      std::string Solver; // "Gurobi" or "HiGHS"
      bool Available;
      bool Success;
      double WCET;
      double SolveTime; // in milliseconds
      std::string Status;
    };

    std::vector<UnifiedSolverResult> AllResults;

    // Collect Legacy results (already computed above)
    // We need to re-run them here with timing
#ifdef ENABLE_GUROBI
    {
      auto Solver = std::make_unique<GurobiSolver>();
      UnifiedSolverResult SR;
      SR.Type = "Legacy";
      SR.Solver = "Gurobi";
      SR.Available = Solver->isAvailable();

      if (SR.Available) {
        auto StartTime = std::chrono::high_resolution_clock::now();
        ILPResult Res = Solver->solveWCET(TAR.MASG, EntryNodeId, ExitNodeId,
                                          EmptyLoopBoundMap);
        auto EndTime = std::chrono::high_resolution_clock::now();

        SR.Success = Res.Success;
        SR.WCET = Res.ObjectiveValue;
        SR.SolveTime =
            std::chrono::duration<double, std::milli>(EndTime - StartTime)
                .count();
        SR.Status = Res.Success ? "Optimal" : "Failed";
      } else {
        SR.Success = false;
        SR.WCET = 0.0;
        SR.SolveTime = 0.0;
        SR.Status = "No license";
      }
      AllResults.push_back(SR);
    }
#endif

#ifdef ENABLE_HIGHS
    {
      auto Solver = std::make_unique<HighsSolver>();
      UnifiedSolverResult SR;
      SR.Type = "Legacy";
      SR.Solver = "HiGHS";
      SR.Available = Solver->isAvailable();

      if (SR.Available) {
        auto StartTime = std::chrono::high_resolution_clock::now();
        ILPResult Res = Solver->solveWCET(TAR.MASG, EntryNodeId, ExitNodeId,
                                          EmptyLoopBoundMap);
        auto EndTime = std::chrono::high_resolution_clock::now();

        SR.Success = Res.Success;
        SR.WCET = Res.ObjectiveValue;
        SR.SolveTime =
            std::chrono::duration<double, std::milli>(EndTime - StartTime)
                .count();
        SR.Status = Res.Success ? "Optimal" : "Failed";
      } else {
        SR.Success = false;
        SR.WCET = 0.0;
        SR.SolveTime = 0.0;
        SR.Status = "Not available";
      }
      AllResults.push_back(SR);
    }
#endif

    // Run Abstract solvers with timing
#ifdef ENABLE_GUROBI
    {
      auto Solver = std::make_unique<AbstractGurobiSolver>();
      UnifiedSolverResult SR;
      SR.Type = "Abstract";
      SR.Solver = "Gurobi";
      SR.Available = true;

      auto StartTime = std::chrono::high_resolution_clock::now();
      auto Res = Solver->solveWCET(AnalysisWorker.getGraph());
      auto EndTime = std::chrono::high_resolution_clock::now();

      SR.Success = (Res.WCET > 0);
      SR.WCET = Res.WCET;
      SR.SolveTime =
          std::chrono::duration<double, std::milli>(EndTime - StartTime)
              .count();
      SR.Status = SR.Success ? "Optimal" : "Failed";
      AllResults.push_back(SR);
    }
#endif

#ifdef ENABLE_HIGHS
    {
      auto Solver = std::make_unique<AbstractHighsSolver>();
      UnifiedSolverResult SR;
      SR.Type = "Abstract";
      SR.Solver = "HiGHS";
      SR.Available = true;

      auto StartTime = std::chrono::high_resolution_clock::now();
      auto Res = Solver->solveWCET(AnalysisWorker.getGraph());
      auto EndTime = std::chrono::high_resolution_clock::now();

      SR.Success = (Res.WCET > 0);
      SR.WCET = Res.WCET;
      SR.SolveTime =
          std::chrono::duration<double, std::milli>(EndTime - StartTime)
              .count();
      SR.Status = SR.Success ? "Optimal" : "Failed";
      AllResults.push_back(SR);
    }
#endif

    // Print unified comparison table
    outs() << "\n=== Unified Solver Comparison Table ===\n";
    outs() << "+----------+-----------+------------+---------+-------------+---"
              "-------------+\n";
    outs() << "| Type     | Solver    | Available  | Success | WCET (cyc)  | "
              "Time (ms)      |\n";
    outs() << "+----------+-----------+------------+---------+-------------+---"
              "-------------+\n";

    for (const auto &SR : AllResults) {
      outs() << "| " << format("%-8s", SR.Type.c_str());
      outs() << " | " << format("%-9s", SR.Solver.c_str());
      outs() << " | " << format("%-10s", SR.Available ? "Yes" : "No");
      outs() << " | " << format("%-7s", SR.Success ? "Yes" : "No");
      outs() << " | " << format("%11.0f", SR.WCET);
      outs() << " | " << format("%14.3f", SR.SolveTime);
      outs() << " |\n";
    }

    outs() << "+----------+-----------+------------+---------+-------------+---"
              "-------------+\n";

    // Find fastest successful solver for each type
    double FastestLegacy = std::numeric_limits<double>::max();
    double FastestAbstract = std::numeric_limits<double>::max();
    std::string FastestLegacySolver, FastestAbstractSolver;

    for (const auto &SR : AllResults) {
      if (SR.Available && SR.Success) {
        if (SR.Type == "Legacy" && SR.SolveTime < FastestLegacy) {
          FastestLegacy = SR.SolveTime;
          FastestLegacySolver = SR.Solver;
        }
        if (SR.Type == "Abstract" && SR.SolveTime < FastestAbstract) {
          FastestAbstract = SR.SolveTime;
          FastestAbstractSolver = SR.Solver;
        }
      }
    }

    outs() << "\nFastest Legacy solver:   " << FastestLegacySolver << " ("
           << format("%.3f", FastestLegacy) << " ms)\n";
    outs() << "Fastest Abstract solver: " << FastestAbstractSolver << " ("
           << format("%.3f", FastestAbstract) << " ms)\n";

    // Check if all WCETs match
    bool AllMatch = true;
    double RefWCET = 0;
    for (const auto &SR : AllResults) {
      if (SR.Success) {
        if (RefWCET == 0) {
          RefWCET = SR.WCET;
        } else if (std::abs(SR.WCET - RefWCET) > 1e-6) {
          AllMatch = false;
          break;
        }
      }
    }

    if (AllMatch && RefWCET > 0) {
      outs() << "\n[SUCCESS] All solvers agree on WCET: "
             << static_cast<unsigned>(RefWCET) << " cycles\n";
    } else {
      outs() << "\n[WARNING] Solvers produced different WCET values!\n";
    }
  } else {
    // Single solver mode for abstract analysis
    std::unique_ptr<AbstractILPSolver> AbstractSolver;
    std::string SolverName;

    if (SolverType == ILPSolverType::Gurobi) {
#ifdef ENABLE_GUROBI
      AbstractSolver = std::make_unique<AbstractGurobiSolver>();
      SolverName = "Gurobi";
#else
      outs() << "Gurobi not available, falling back to HiGHS\n";
#endif
    }

    if (!AbstractSolver && (SolverType == ILPSolverType::HiGHS ||
                            SolverType == ILPSolverType::Auto)) {
#ifdef ENABLE_HIGHS
      AbstractSolver = std::make_unique<AbstractHighsSolver>();
      SolverName = "HiGHS";
#endif
    }

    if (!AbstractSolver) {
#ifdef ENABLE_GUROBI
      AbstractSolver = std::make_unique<AbstractGurobiSolver>();
      SolverName = "Gurobi";
#endif
    }

    if (AbstractSolver) {
      outs() << "Using Abstract ILP solver: " << SolverName << "\n";
      auto AbstractResult =
          AbstractSolver->solveWCET(AnalysisWorker.getGraph());
      outs() << "New Abstract Analysis WCET: "
             << static_cast<unsigned>(AbstractResult.WCET) << " cycles\n";
      outs() << "Legacy Analysis WCET:       "
             << static_cast<unsigned>(Result.ObjectiveValue) << " cycles\n";

      if (Result.Success) {
        double Diff = std::abs(AbstractResult.WCET - Result.ObjectiveValue);
        if (Diff < 1e-6) {
          outs() << "[SUCCESS] WCET matches!\n";
        } else {
          outs() << "[DIFFERENCE] WCET differs by " << Diff << " cycles\n";
        }
      }
    } else {
      outs() << "No abstract ILP solver available.\n";
    }
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
