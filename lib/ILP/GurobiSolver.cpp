#include "ILP/GurobiSolver.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#ifdef ENABLE_GUROBI
#include <gurobi_c.h>
#endif

#define DEBUG_TYPE "ilp"

namespace llvm {

GurobiSolver::GurobiSolver() : HasLicense(false) {
#ifdef ENABLE_GUROBI
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
#endif
}

GurobiSolver::~GurobiSolver() = default;

bool GurobiSolver::isAvailable() const {
#ifdef ENABLE_GUROBI
  return HasLicense;
#else
  return false;
#endif
}

ILPResult
GurobiSolver::solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
                        unsigned ExitNodeId,
                        const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;

#ifdef ENABLE_GUROBI
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

  // Constraint 3: Flow conservation for all nodes (except entry and exit)
  // Sum of incoming flow = Sum of outgoing flow = execution count
  // For each node i: sum(x_pred) = x_i and sum(x_succ) = x_i
  // Or equivalently: sum(x_pred) - x_i = 0 and x_i - sum(x_succ) = 0
  // Combined: sum(x_pred) = sum(x_succ)
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    // Skip entry and exit nodes
    if (NodeId == EntryNodeId || NodeId == ExitNodeId) {
      continue;
    }

    // In-flow constraint: sum(predecessors) = x_i
    const auto &Preds = N.getPredecessors();
    if (!Preds.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      for (unsigned PredId : Preds) {
        Indices.push_back(NodeToVarIdx[PredId]);
        Coeffs.push_back(1.0);
      }
      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(-1.0);

      std::string ConstrName = "flow_in_" + std::to_string(NodeId);
      error = GRBaddconstr(model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_EQUAL, 0.0, ConstrName.c_str());
      if (error) {
        Result.StatusMessage =
            "Failed to add flow in constraint for node " + std::to_string(NodeId);
        GRBfreemodel(model);
        GRBfreeenv(env);
        return Result;
      }
    }

    // Out-flow constraint: x_i = sum(successors)
    const auto &Succs = N.getSuccessors();
    if (!Succs.empty()) {
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
                           GRB_EQUAL, 0.0, ConstrName.c_str());
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
  // For loop headers: x_header <= N * sum(x_preheader)
  // where preheader edges are edges from outside the loop
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    if (!N.IsLoop) {
      continue;
    }

    auto LoopIt = LoopBoundMap.find(NodeId);
    if (LoopIt == LoopBoundMap.end()) {
      continue;
    }

    unsigned LoopBound = LoopIt->second;

    // Identify back edges vs entry edges
    // A simple heuristic: back edges come from successors with higher or equal
    // node IDs (assuming loop body nodes are after header in the graph)
    // For proper identification, we'd need loop analysis info
    
    // For now, use a simpler constraint: x_header <= LoopBound * entry_flow
    // where entry_flow is 1 for the outermost context
    // This means: x_header <= LoopBound (for single-entry loops)
    
    // More precisely: count back-edges. For each predecessor that is NOT
    // a back-edge (i.e., comes from outside the loop), that's a preheader.
    // Back-edge detection: predecessor is a successor of the header (cyclic)
    
    std::vector<unsigned> PreheaderPreds;
    std::vector<unsigned> BackEdgePreds;
    
    for (unsigned PredId : N.getPredecessors()) {
      // Check if this predecessor is reachable from header (back-edge)
      // Simple heuristic: if predecessor has header as successor, it's a back-edge
      const auto &PredSuccs = MASG.getSuccessors(PredId);
      bool IsBackEdge = false;
      // If the predecessor is a successor of the header in the graph topology,
      // then this edge from pred to header is a back edge
      // Actually, check if pred is in the "body" of the loop
      // Simpler: if pred has a higher node ID, assume it's from within the loop
      if (PredId > NodeId) {
        IsBackEdge = true;
      }
      
      if (IsBackEdge) {
        BackEdgePreds.push_back(PredId);
      } else {
        PreheaderPreds.push_back(PredId);
      }
    }

    // Constraint: x_header <= LoopBound * sum(preheader_flow)
    // Rearranged: x_header - LoopBound * sum(x_preheader) <= 0
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

#else
  Result.StatusMessage = "Gurobi support not compiled in";
#endif

  return Result;
}

} // namespace llvm
