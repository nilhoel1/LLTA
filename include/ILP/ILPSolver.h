#ifndef ILP_SOLVER_H
#define ILP_SOLVER_H

#include "RTTargets/MuArchStateGraph.h"
#include <map>
#include <memory>
#include <string>

namespace llvm {

/// Result of an ILP solve operation
struct ILPResult {
  bool Success;
  double ObjectiveValue;
  std::map<unsigned, double> NodeExecutionCounts;
  std::map<std::pair<unsigned, unsigned>, double> EdgeExecutionCounts;
  std::string StatusMessage;

  ILPResult()
      : Success(false), ObjectiveValue(0.0), StatusMessage("Not solved") {}
};

/// Abstract base class for ILP solvers
class ILPSolver {
public:
  virtual ~ILPSolver() = default;

  /// Solve the WCET ILP problem for the given MuArchStateGraph
  /// Entry and exit nodes must be identified in the graph
  /// @param MASG The microarchitectural state graph
  /// @param EntryNodeId The ID of the entry node (NrTakenNode_entry = 1)
  /// @param ExitNodeId The ID of the exit node (NrTakenNode_exit = 1)
  /// @param LoopBoundMap Map from loop header node IDs to their upper bounds
  /// @return The result of the ILP solve operation
  virtual ILPResult
  solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
            unsigned ExitNodeId,
            const std::map<unsigned, unsigned> &LoopBoundMap) = 0;

  /// Get the name of the solver
  virtual std::string getName() const = 0;

  /// Check if the solver is available and licensed
  virtual bool isAvailable() const = 0;
};

/// Enum for solver type selection
enum class ILPSolverType { Auto, Gurobi, HiGHS, All };

/// Factory function to create an ILP solver based on the requested type
/// @param Type The type of solver to create
/// @return A unique pointer to the solver, or nullptr if unavailable
std::unique_ptr<ILPSolver> createILPSolver(ILPSolverType Type);

/// Parse solver type from string
/// @param SolverName The name of the solver ("auto", "gurobi", "highs")
/// @return The corresponding solver type
ILPSolverType parseILPSolverType(const std::string &SolverName);

} // namespace llvm

#endif // ILP_SOLVER_H
