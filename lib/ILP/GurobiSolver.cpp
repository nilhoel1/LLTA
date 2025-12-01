#include "ILP/GurobiSolver.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#ifdef ENABLE_GUROBI
#include <gurobi_c.h>
#endif

#define DEBUG_TYPE "ilp"

namespace llvm {

#ifdef ENABLE_GUROBI

GurobiSolver::GurobiSolver() : HasLicense(false) {
  // Try to create a Gurobi environment to check license
  GRBenv *env = nullptr;
  int error = GRBloadenv(&env, nullptr);
  if (error == 0 && env != nullptr) {
    HasLicense = true;
    GRBfreeenv(env);
  } else {
    HasLicense = false;
    DEBUG_WITH_TYPE("ilp", dbgs() << "Gurobi license check failed with error "
                                  << error << "\n");
  }
}

GurobiSolver::~GurobiSolver() = default;

bool GurobiSolver::isAvailable() const {
  return HasLicense;
}

ILPResult
GurobiSolver::solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
                        unsigned ExitNodeId,
                        const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;

  GRBenv *env = nullptr;
  GRBmodel *model = nullptr;
  int error = 0;

  // Create environment
  error = GRBloadenv(&env, nullptr);
  if (error) {
    Result.StatusMessage = "Failed to create Gurobi environment";
    return Result;
  }

  // Suppress Gurobi output
  GRBsetintparam(env, GRB_INT_PAR_OUTPUTFLAG, 0);

  // Create an empty model
  error = GRBnewmodel(env, &model, "WCET", 0, nullptr, nullptr, nullptr, nullptr,
                      nullptr);
  if (error) {
    Result.StatusMessage = "Failed to create Gurobi model";
    GRBfreeenv(env);
    return Result;
  }

  // Set objective to maximize
  GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MAXIMIZE);

  const auto &Nodes = MASG.getNodes();
  int NumNodes = Nodes.size();

  // Create a mapping from node ID to variable index
  std::map<unsigned, int> NodeToVarIdx;
  std::vector<unsigned> VarIdxToNode;
  int VarIdx = 0;
  for (const auto &NodePair : Nodes) {
    NodeToVarIdx[NodePair.first] = VarIdx;
    VarIdxToNode.push_back(NodePair.first);
    VarIdx++;
  }

  // Add variables: x_i = execution count of node i (integer >= 0)
  // Objective coefficient = UpperBoundCycles for each node
  std::vector<double> Obj(NumNodes);
  std::vector<double> Lb(NumNodes, 0.0);
  std::vector<double> Ub(NumNodes, GRB_INFINITY);
  std::vector<char> VTypes(NumNodes, GRB_INTEGER);

  for (const auto &NodePair : Nodes) {
    int Idx = NodeToVarIdx[NodePair.first];
    const Node &N = NodePair.second;
    Obj[Idx] = N.getState().getUpperBoundCycles();
  }

  // Add variables to model
  error = GRBaddvars(model, NumNodes, 0, nullptr, nullptr, nullptr, Obj.data(),
                     Lb.data(), Ub.data(), VTypes.data(), nullptr);
  if (error) {
    Result.StatusMessage = "Failed to add variables to Gurobi model";
    GRBfreemodel(model);
    GRBfreeenv(env);
    return Result;
  }

  // Update model to integrate new variables
  error = GRBupdatemodel(model);
  if (error) {
    Result.StatusMessage = "Failed to update Gurobi model";
    GRBfreemodel(model);
    GRBfreeenv(env);
    return Result;
  }

  // Constraint 1: NrTakenNode_entry = 1
  {
    int EntryIdx = NodeToVarIdx[EntryNodeId];
    double Coeff = 1.0;
    error = GRBaddconstr(model, 1, &EntryIdx, &Coeff, GRB_EQUAL, 1.0,
                         "entry_constraint");
    if (error) {
      Result.StatusMessage = "Failed to add entry constraint";
      GRBfreemodel(model);
      GRBfreeenv(env);
      return Result;
    }
  }

  // Constraint 2: NrTakenNode_exit = 1
  {
    int ExitIdx = NodeToVarIdx[ExitNodeId];
    double Coeff = 1.0;
    error = GRBaddconstr(model, 1, &ExitIdx, &Coeff, GRB_EQUAL, 1.0,
                         "exit_constraint");
    if (error) {
      Result.StatusMessage = "Failed to add exit constraint";
      GRBfreemodel(model);
      GRBfreeenv(env);
      return Result;
    }
  }

  // Constraint 3: Flow conservation for all nodes
  // For non-entry nodes: x_i <= sum(x_pred) - we can only execute i if we came from somewhere
  // For non-exit nodes: x_i <= sum(x_succ) - after executing i, we must go somewhere
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    const auto &Preds = N.getPredecessors();
    const auto &Succs = N.getSuccessors();

    // For non-entry nodes: x_i <= sum(x_pred)
    // Rearranged: x_i - sum(x_pred) <= 0
    if (NodeId != EntryNodeId && !Preds.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(1.0);

      for (unsigned PredId : Preds) {
        Indices.push_back(NodeToVarIdx[PredId]);
        Coeffs.push_back(-1.0);
      }

      std::string ConstrName = "flow_in_" + std::to_string(NodeId);
      error = GRBaddconstr(model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_LESS_EQUAL, 0.0, ConstrName.c_str());
      if (error) {
        Result.StatusMessage =
            "Failed to add flow in constraint for node " + std::to_string(NodeId);
        GRBfreemodel(model);
        GRBfreeenv(env);
        return Result;
      }
    }

    // For non-exit nodes: x_i <= sum(x_succ)
    // Rearranged: x_i - sum(x_succ) <= 0
    if (NodeId != ExitNodeId && !Succs.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(1.0);

      for (unsigned SuccId : Succs) {
        Indices.push_back(NodeToVarIdx[SuccId]);
        Coeffs.push_back(-1.0);
      }

      std::string ConstrName = "flow_out_" + std::to_string(NodeId);
      error = GRBaddconstr(model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_LESS_EQUAL, 0.0, ConstrName.c_str());
      if (error) {
        Result.StatusMessage =
            "Failed to add flow out constraint for node " + std::to_string(NodeId);
        GRBfreemodel(model);
        GRBfreeenv(env);
        return Result;
      }
    }
  }

  // Constraint 4: Loop bound constraints
  // Loop headers are marked with IsLoop=true and have UpperLoopBound set
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    if (!N.IsLoop) {
      continue;
    }

    unsigned LoopBound = N.UpperLoopBound;

    // Back-edge detection heuristic based on node ID ordering.
    // Assumes nodes within a loop body have higher IDs than the loop header.
    std::vector<unsigned> PreheaderPreds;
    std::vector<unsigned> BackEdgePreds;
    
    for (unsigned PredId : N.getPredecessors()) {
      bool IsBackEdge = (PredId > NodeId);
      if (IsBackEdge) {
        BackEdgePreds.push_back(PredId);
      } else {
        PreheaderPreds.push_back(PredId);
      }
    }

    // Constraint: x_header <= LoopBound * sum(preheader_flow)
    if (!PreheaderPreds.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(1.0);

      for (unsigned PrehId : PreheaderPreds) {
        Indices.push_back(NodeToVarIdx[PrehId]);
        Coeffs.push_back(-static_cast<double>(LoopBound));
      }

      std::string ConstrName = "loop_bound_" + std::to_string(NodeId);
      error = GRBaddconstr(model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_LESS_EQUAL, 0.0, ConstrName.c_str());
      if (error) {
        Result.StatusMessage =
            "Failed to add loop bound constraint for node " + std::to_string(NodeId);
        GRBfreemodel(model);
        GRBfreeenv(env);
        return Result;
      }
    } else {
      // No preheader found, use absolute bound
      int HeaderIdx = NodeToVarIdx[NodeId];
      double Coeff = 1.0;
      std::string ConstrName = "loop_bound_abs_" + std::to_string(NodeId);
      error = GRBaddconstr(model, 1, &HeaderIdx, &Coeff, GRB_LESS_EQUAL,
                           static_cast<double>(LoopBound), ConstrName.c_str());
      if (error) {
        Result.StatusMessage =
            "Failed to add absolute loop bound constraint for node " +
            std::to_string(NodeId);
        GRBfreemodel(model);
        GRBfreeenv(env);
        return Result;
      }
    }
  }

  // Write model to file for debugging
  error = GRBwrite(model, "gurobi_wcet_model.lp");
  if (error) {
    outs() << "Warning: Failed to write Gurobi model to file\n";
  }
  
  // Also write in MPS format
  error = GRBwrite(model, "gurobi_wcet_model.mps");
  if (error) {
    outs() << "Warning: Failed to write Gurobi MPS model to file\n";
  }

  // Optimize model
  error = GRBoptimize(model);
  if (error) {
    Result.StatusMessage = "Gurobi optimization failed";
    GRBfreemodel(model);
    GRBfreeenv(env);
    return Result;
  }

  // Check optimization status
  int OptStatus;
  GRBgetintattr(model, GRB_INT_ATTR_STATUS, &OptStatus);

  if (OptStatus == GRB_OPTIMAL) {
    Result.Success = true;
    GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &Result.ObjectiveValue);
    Result.StatusMessage = "Optimal solution found";

    // Get variable values
    std::vector<double> Values(NumNodes);
    GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, NumNodes, Values.data());

    for (int i = 0; i < NumNodes; i++) {
      Result.NodeExecutionCounts[VarIdxToNode[i]] = Values[i];
    }
  } else if (OptStatus == GRB_INFEASIBLE) {
    Result.StatusMessage = "Model is infeasible";
  } else if (OptStatus == GRB_UNBOUNDED) {
    Result.StatusMessage = "Model is unbounded";
  } else {
    Result.StatusMessage =
        "Optimization ended with status " + std::to_string(OptStatus);
  }

  // Clean up
  GRBfreemodel(model);
  GRBfreeenv(env);

  return Result;
}

#else // !ENABLE_GUROBI

GurobiSolver::GurobiSolver() : HasLicense(false) {}

GurobiSolver::~GurobiSolver() = default;

bool GurobiSolver::isAvailable() const {
  return false;
}

ILPResult
GurobiSolver::solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
                        unsigned ExitNodeId,
                        const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;
  Result.StatusMessage = "Gurobi support not compiled in";
  return Result;
}

#endif // ENABLE_GUROBI

} // namespace llvm
