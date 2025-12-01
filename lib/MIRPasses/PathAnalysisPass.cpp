#include "MIRPasses/PathAnalysisPass.h"
#include "ILP/ILPSolver.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/CallGraph.h"
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
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <memory>
#include <vector>

namespace llvm {

char PathAnalysisPass::ID = 0;

/**
 * @brief Construct a new PathAnalysisPass object
 *
 * @param TAR TimingAnalysisResults reference
 */
PathAnalysisPass::PathAnalysisPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

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
    if (!StartFunctionName.empty() && F->getName().compare(StartFunctionName) == 0) {
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
bool PathAnalysisPass::finalizePathAnalysis(MuArchStateGraph &MASG) {
  outs() << "\n=== Path Analysis: Computing WCET via ILP ===\n";

  // Parse solver type from command-line option
  ILPSolverType SolverType = parseILPSolverType(ILPSolverOption);

  // Create the solver
  auto Solver = createILPSolver(SolverType);
  if (!Solver) {
    outs() << "Error: No ILP solver available. Cannot compute WCET.\n";
    return false;
  }

  outs() << "Using ILP solver: " << Solver->getName() << "\n";

  // Find entry and exit nodes
  // Entry node: node with name "Entry" or the first node with no predecessors
  // Exit node: node with name "Exit" or the last node with no successors
  unsigned EntryNodeId = UINT_MAX;
  unsigned ExitNodeId = UINT_MAX;

  const auto &Nodes = MASG.getNodes();

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
    outs() << "Error: Could not identify entry and/or exit nodes in the graph.\n";
    return false;
  }

  outs() << "Entry node ID: " << EntryNodeId << "\n";
  outs() << "Exit node ID: " << ExitNodeId << "\n";
  outs() << "Total nodes: " << Nodes.size() << "\n";

  // Build loop bound map from node IDs to bounds
  std::map<unsigned, unsigned> LoopBoundMap;
  for (const auto &NodePair : Nodes) {
    const Node &N = NodePair.second;
    if (N.IsLoop) {
      LoopBoundMap[NodePair.first] = N.UpperLoopBound;
      outs() << "Loop node " << NodePair.first << " (" << N.Name
             << ") has bound: " << N.UpperLoopBound << "\n";
    }
  }

  // Solve the ILP
  outs() << "\nSolving WCET ILP...\n";
  ILPResult Result = Solver->solveWCET(MASG, EntryNodeId, ExitNodeId, LoopBoundMap);

  // Report results
  outs() << "\n=== WCET Analysis Results ===\n";
  outs() << "Status: " << Result.StatusMessage << "\n";

  if (Result.Success) {
    outs() << "WCET (worst-case execution time): " << static_cast<unsigned>(Result.ObjectiveValue)
           << " cycles\n";

    if (DebugPrints) {
      outs() << "\nNode execution counts:\n";
      for (const auto &[NodeId, Count] : Result.NodeExecutionCounts) {
        if (Count > 0) {
          const Node &N = Nodes.at(NodeId);
          outs() << "  Node " << NodeId << " (" << N.Name << "): "
                 << static_cast<unsigned>(Count) << " times, "
                 << N.getState().getUpperBoundCycles() << " cycles/exec\n";
        }
      }
    }

    return true;
  } else {
    outs() << "Failed to compute WCET.\n";
    return false;
  }
}

/**
 * @brief Iterates over MachineFunction and performs path analysis
 *
 * @param F MachineFunction to analyze
 * @return true if the function was modified (always false for analysis)
 */
bool PathAnalysisPass::runOnMachineFunction(MachineFunction &F) {
  if (StartFunctionName != "")
    FoundStartingFunction = true;
  if (!CG) {
    CG = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }
  if (!FoundStartingFunction) {
    StartingFunction = getStartingFunction(*CG);
    // Get the starting function if the StartingFunctionName is empty
    if (!StartingFunction) {
      outs() << "No StartingFunction found\n";
    }
    assert(StartingFunction && "StartingFunction is null");
  }
  // Only continue when StartFunction is not set as parameter.
  if (!(&F.getFunction() == StartingFunction) && StartFunctionName == "") {
    return false;
  }
  if (StartFunctionName != F.getName() && StartFunctionName != "") {
    return false;
  }
  outs() << "Starting Function: " << F.getName() << "\n";
  outs() << "Should Be: " << StartFunctionName << "\n";

  // Get MachineModuleInfo
  auto *MMI = &getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  // Get the MachineLoopInfo analysis results
  auto &MLWP = getAnalysis<MachineLoopInfoWrapperPass>();
  (void)MLWP; // Suppress unused variable warning

  // Get the Latency analysis results
  auto MBBLatencyMap = TAR.getMBBLatencyMap();

  // Print Loop Bounds
  outs() << "Aggregated Loop Bounds:\n";
  for (auto const& [MBB, Bound] : TAR.LoopBoundMap) {
      outs() << "MBB: " << MBB->getName() << " Bound: " << Bound << "\n";
  }

  // Check if this is the last function to finalize and solve ILP
  bool IsLast = false;
  const Function *LastF = nullptr;
  for (auto &Func : MMI->getModule()->getFunctionList()) {
    if (!Func.isDeclaration()) {
      LastF = &Func;
    }
  }

  if (LastF == &F.getFunction()) {
    IsLast = true;
  }

  // Solve the WCET ILP on the finalized MuArchStateGraph
  if (IsLast) {
    finalizePathAnalysis(TAR.MASG);
  }

  return false;
}

MachineFunctionPass *createPathAnalysisPass(TimingAnalysisResults &TAR) {
  return new PathAnalysisPass(TAR);
}
} // namespace llvm
