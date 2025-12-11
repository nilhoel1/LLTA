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
  GRBenv *Env = nullptr;
  int Error = GRBloadenv(&Env, nullptr);
  if (Error == 0 && Env != nullptr) {
    HasLicense = true;
    GRBfreeenv(Env);
  } else {
    HasLicense = false;
    DEBUG_WITH_TYPE("ilp", dbgs() << "Gurobi license check failed with error "
                                  << Error << "\n");
  }
}

GurobiSolver::~GurobiSolver() = default;

bool GurobiSolver::isAvailable() const { return HasLicense; }

ILPResult
GurobiSolver::solveWCET(const ProgramGraph &MASG, unsigned EntryNodeId,
                        unsigned ExitNodeId,
                        const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;

  GRBenv *Env = nullptr;
  GRBmodel *Model = nullptr;
  int Error = 0;

  // Create environment
  Error = GRBloadenv(&Env, nullptr);
  if (Error) {
    Result.StatusMessage = "Failed to create Gurobi environment";
    return Result;
  }

  // Suppress Gurobi output
  GRBsetintparam(Env, GRB_INT_PAR_OUTPUTFLAG, 0);

  // Create an empty model
  Error = GRBnewmodel(Env, &Model, "WCET", 0, nullptr, nullptr, nullptr,
                      nullptr, nullptr);
  if (Error) {
    Result.StatusMessage = "Failed to create Gurobi model";
    GRBfreeenv(Env);
    return Result;
  }

  // Set objective to maximize
  GRBsetintattr(Model, GRB_INT_ATTR_MODELSENSE, GRB_MAXIMIZE);

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
  // Create a mapping from edge to variable index

  // Each edge is identified by (SourceNodeId, TargetNodeId)
  std::map<std::pair<unsigned, unsigned>, int> EdgeToVarIdx;
  std::vector<std::pair<unsigned, unsigned>> VarIdxToEdge;
  int EdgeVarIdx = NumNodes; // Edge variables start after node variables
  int NumEdges = 0;

  for (const auto &NodePair : Nodes) {
    unsigned SourceId = NodePair.first;
    const Node &N = NodePair.second;
    for (unsigned TargetId : N.getSuccessors()) {
      EdgeToVarIdx[{SourceId, TargetId}] = EdgeVarIdx;
      VarIdxToEdge.push_back({SourceId, TargetId});
      EdgeVarIdx++;
      NumEdges++;
    }
  }

  // Add variables: x_i = execution count of node i (integer >= 0)
  // Objective coefficient = UpperBoundCycles for each node
  std::vector<double> Obj(NumNodes);
  std::vector<double> Lb(NumNodes, 0.0);
  std::vector<double> Ub(NumNodes, GRB_INFINITY);
  std::vector<char> VTypes(NumNodes, GRB_INTEGER);
  std::vector<std::string> NodeNames(NumNodes);
  std::vector<char *> NodeNamePtrs(NumNodes);

  for (const auto &NodePair : Nodes) {
    int Idx = NodeToVarIdx[NodePair.first];
    const Node &N = NodePair.second;
    Obj[Idx] = N.getState().getUpperBoundCycles();
    NodeNames[Idx] = "N" + std::to_string(NodePair.first);
    NodeNamePtrs[Idx] = const_cast<char *>(NodeNames[Idx].c_str());
  }

  // Add variables to model
  Error = GRBaddvars(Model, NumNodes, 0, nullptr, nullptr, nullptr, Obj.data(),
                     Lb.data(), Ub.data(), VTypes.data(), NodeNamePtrs.data());
  if (Error) {
    Result.StatusMessage = "Failed to add variables to Gurobi model";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Update model to integrate new variables
  Error = GRBupdatemodel(Model);
  if (Error) {
    Result.StatusMessage = "Failed to update Gurobi model";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Add edge variables: e_{i,j} = execution count of edge (i,j) (integer >= 0)
  // Objective coefficient = 0 for edge variables (they don't contribute to WCET
  // directly)
  std::vector<double> EdgeObj(NumEdges, 0.0);
  std::vector<double> EdgeLb(NumEdges, 0.0);
  std::vector<double> EdgeUb(NumEdges, GRB_INFINITY);
  std::vector<char> EdgeVTypes(NumEdges, GRB_INTEGER);
  std::vector<std::string> EdgeNames(NumEdges);
  std::vector<char *> EdgeNamePtrs(NumEdges);

  for (int I = 0; I < NumEdges; ++I) {
    EdgeNames[I] = "E" + std::to_string(I);
    EdgeNamePtrs[I] = const_cast<char *>(EdgeNames[I].c_str());
  }

  // Add edge variables to model
  Error = GRBaddvars(Model, NumEdges, 0, nullptr, nullptr, nullptr,
                     EdgeObj.data(), EdgeLb.data(), EdgeUb.data(),
                     EdgeVTypes.data(), EdgeNamePtrs.data());
  if (Error) {
    Result.StatusMessage = "Failed to add edge variables to Gurobi model";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Update model to integrate new variables
  Error = GRBupdatemodel(Model);
  if (Error) {
    Result.StatusMessage = "Failed to update Gurobi model";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Constraint 1: NrTakenNode_entry = 1
  {
    int EntryIdx = NodeToVarIdx[EntryNodeId];
    double Coeff = 1.0;
    Error = GRBaddconstr(Model, 1, &EntryIdx, &Coeff, GRB_EQUAL, 1.0,
                         "entry_constraint");
    if (Error) {
      Result.StatusMessage = "Failed to add entry constraint";
      GRBfreemodel(Model);
      GRBfreeenv(Env);
      return Result;
    }
  }

  // Constraint 2: NrTakenNode_exit = 1
  {
    int ExitIdx = NodeToVarIdx[ExitNodeId];
    double Coeff = 1.0;
    Error = GRBaddconstr(Model, 1, &ExitIdx, &Coeff, GRB_EQUAL, 1.0,
                         "exit_constraint");
    if (Error) {
      Result.StatusMessage = "Failed to add exit constraint";
      GRBfreemodel(Model);
      GRBfreeenv(Env);
      return Result;
    }
  }

  // Constraint 3: Flow conservation with edge variables
  // For each node i: sum(incoming edges) = x_i and sum(outgoing edges) = x_i
  // This ensures proper flow through the graph
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    const auto &Preds = N.getPredecessors();
    const auto &Succs = N.getSuccessors();

    // Incoming flow conservation: sum(e_{pred,i}) = x_i
    if (!Preds.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      // Add node variable x_i with coefficient -1
      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(-1.0);

      // Add incoming edge variables with coefficient +1
      for (unsigned PredId : Preds) {
        auto EdgeKey = std::make_pair(PredId, NodeId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          Indices.push_back(EdgeToVarIdx[EdgeKey]);
          Coeffs.push_back(1.0);
        }
      }

      std::string ConstrName = "flow_in_" + std::to_string(NodeId);
      Error = GRBaddconstr(Model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_EQUAL, 0.0, ConstrName.c_str());
      if (Error) {
        Result.StatusMessage =
            "Failed to add incoming flow constraint for node " +
            std::to_string(NodeId);
        GRBfreemodel(Model);
        GRBfreeenv(Env);
        return Result;
      }
    }

    // Outgoing flow conservation: sum(e_{i,succ}) = x_i
    if (!Succs.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      // Add node variable x_i with coefficient -1
      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(-1.0);

      // Add outgoing edge variables with coefficient +1
      for (unsigned SuccId : Succs) {
        auto EdgeKey = std::make_pair(NodeId, SuccId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          Indices.push_back(EdgeToVarIdx[EdgeKey]);
          Coeffs.push_back(1.0);
        }
      }

      std::string ConstrName = "flow_out_" + std::to_string(NodeId);
      Error = GRBaddconstr(Model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_EQUAL, 0.0, ConstrName.c_str());
      if (Error) {
        Result.StatusMessage =
            "Failed to add outgoing flow constraint for node " +
            std::to_string(NodeId);
        GRBfreemodel(Model);
        GRBfreeenv(Env);
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

    // Back-edge detection using explicit backedge info from graph construction
    std::vector<unsigned> PreheaderPreds;
    std::vector<unsigned> BackEdgePreds;

    const auto &BackEdges = N.BackEdgePredecessors;

    for (unsigned PredId : N.getPredecessors()) {
      if (BackEdges.count(PredId)) {
        BackEdgePreds.push_back(PredId);
      } else {
        PreheaderPreds.push_back(PredId);
      }
    }

    // Fallback to heuristic if no backedges found but node is a loop header
    // This handles cases where MachineLoopInfo wasn't available or failed
    if (BackEdgePreds.empty() && !N.getPredecessors().empty()) {
      PreheaderPreds.clear(); // Clear potentially incorrect preheaders
      for (unsigned PredId : N.getPredecessors()) {
        bool IsBackEdge = (PredId > NodeId);
        if (IsBackEdge) {
          BackEdgePreds.push_back(PredId);
        } else {
          PreheaderPreds.push_back(PredId);
        }
      }
      // Clear vectors if we re-populated them
      if (BackEdgePreds.empty()) {
        // If heuristic also failed, assume all preds are preheaders (safe
        // default?) Or maybe we should assume all preds < NodeId are preheaders
        PreheaderPreds.clear();
        for (unsigned PredId : N.getPredecessors()) {
          PreheaderPreds.push_back(PredId);
        }
      }
    }

    // Constraint: x_header <= LoopBound * sum(preheader_flow)
    if (!PreheaderPreds.empty()) {
      std::vector<int> Indices;
      std::vector<double> Coeffs;

      Indices.push_back(NodeToVarIdx[NodeId]);
      Coeffs.push_back(1.0);

      for (unsigned PrehId : PreheaderPreds) {
        // Use edge variable for flow from preheader to header
        // x_header <= LoopBound * sum(flow(preheader -> header))
        // flow(preheader -> header) is the edge variable E_{preheader, header}

        auto EdgeKey = std::make_pair(PrehId, NodeId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          Indices.push_back(EdgeToVarIdx[EdgeKey]);
          Coeffs.push_back(-static_cast<double>(LoopBound));
        } else {
          // Fallback to node variable if edge not found (should not happen)
          Indices.push_back(NodeToVarIdx[PrehId]);
          Coeffs.push_back(-static_cast<double>(LoopBound));
        }
      }

      std::string ConstrName = "loop_bound_" + std::to_string(NodeId);
      Error = GRBaddconstr(Model, Indices.size(), Indices.data(), Coeffs.data(),
                           GRB_LESS_EQUAL, 0.0, ConstrName.c_str());
      if (Error) {
        Result.StatusMessage = "Failed to add loop bound constraint for node " +
                               std::to_string(NodeId);
        GRBfreemodel(Model);
        GRBfreeenv(Env);
        return Result;
      }
    } else {
      // No preheader found, use absolute bound
      int HeaderIdx = NodeToVarIdx[NodeId];
      double Coeff = 1.0;
      std::string ConstrName = "loop_bound_abs_" + std::to_string(NodeId);
      Error = GRBaddconstr(Model, 1, &HeaderIdx, &Coeff, GRB_LESS_EQUAL,
                           static_cast<double>(LoopBound), ConstrName.c_str());
      if (Error) {
        Result.StatusMessage =
            "Failed to add absolute loop bound constraint for node " +
            std::to_string(NodeId);
        GRBfreemodel(Model);
        GRBfreeenv(Env);
        return Result;
      }
    }
  }

  // Write model to file for debugging
  Error = GRBwrite(Model, "gurobi_wcet_model.lp");
  if (Error) {
    outs() << "Warning: Failed to write Gurobi model to file\n";
  }

  // Also write in MPS format
  Error = GRBwrite(Model, "gurobi_wcet_model.mps");
  if (Error) {
    outs() << "Warning: Failed to write Gurobi MPS model to file\n";
  }

  // Optimize model
  Error = GRBoptimize(Model);
  if (Error) {
    Result.StatusMessage = "Gurobi optimization failed";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Check optimization status
  int OptStatus;
  GRBgetintattr(Model, GRB_INT_ATTR_STATUS, &OptStatus);

  if (OptStatus == GRB_OPTIMAL) {
    Result.Success = true;
    GRBgetdblattr(Model, GRB_DBL_ATTR_OBJVAL, &Result.ObjectiveValue);
    Result.StatusMessage = "Optimal solution found";

    // Get variable values (both nodes and edges)
    int TotalVars = NumNodes + NumEdges;
    std::vector<double> Values(TotalVars);
    GRBgetdblattrarray(Model, GRB_DBL_ATTR_X, 0, TotalVars, Values.data());

    // Extract node execution counts
    for (int I = 0; I < NumNodes; I++) {
      Result.NodeExecutionCounts[VarIdxToNode[I]] = Values[I];
    }

    // Extract edge execution counts
    for (int I = 0; I < NumEdges; I++) {
      Result.EdgeExecutionCounts[VarIdxToEdge[I]] = Values[NumNodes + I];
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
  GRBfreemodel(Model);
  GRBfreeenv(Env);

  return Result;
}

#else // !ENABLE_GUROBI

GurobiSolver::GurobiSolver() : HasLicense(false) {}

GurobiSolver::~GurobiSolver() = default;

bool GurobiSolver::isAvailable() const { return false; }

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
