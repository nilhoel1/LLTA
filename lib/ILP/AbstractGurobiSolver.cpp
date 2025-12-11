#include "ILP/AbstractGurobiSolver.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

// Check if Gurobi is enabled
#ifdef ENABLE_GUROBI
#include "gurobi_c.h"
#endif

#define DEBUG_TYPE "abstract-gurobi-solver"

namespace llvm {

AbstractGurobiSolver::AbstractGurobiSolver() {}

AbstractGurobiSolver::~AbstractGurobiSolver() {}

AbstractILPResult
AbstractGurobiSolver::solveWCET(const AbstractStateGraph &ASG) {
  AbstractILPResult Result;
  Result.WCET = 0.0;

#ifdef ENABLE_GUROBI
  GRBenv *Env = nullptr;
  GRBmodel *Model = nullptr;
  int Error = 0;

  // Create environment
  Error = GRBloadenv(&Env, "abstract_ilp_solver.log");
  if (Error) {
    errs() << "Failed to create Gurobi environment: " << GRBgeterrormsg(Env)
           << "\n";
    return Result;
  }

  // Suppress output
  GRBsetintparam(Env, GRB_INT_PAR_OUTPUTFLAG, 0);

  // Create empty model
  Error = GRBnewmodel(Env, &Model, "AbstractWCET", 0, nullptr, nullptr, nullptr,
                      nullptr, nullptr);
  if (Error) {
    errs() << "Failed to create Gurobi model\n";
    GRBfreeenv(Env);
    return Result;
  }

  // Set maximization
  GRBsetintattr(Model, GRB_INT_ATTR_MODELSENSE, GRB_MAXIMIZE);

  // Map NodeID -> VarIdx
  std::map<unsigned, int> NodeToVarIdx;
  int VarIdx = 0;

  // 1. Add Node Variables
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    const auto &Node = NodePair.second;

    NodeToVarIdx[U] = VarIdx++;

    std::string Name = "x_" + std::to_string(U);
    double Obj = Node->Cost;
    double Lb = 0.0;
    double Ub = GRB_INFINITY;
    char VType = GRB_CONTINUOUS; // Or INTEGER? Abstract states maybe continuous
                                 // flow? WCET usually integer counts but
                                 // continuous is relaxation.
    // Let's use CONTINUOUS for now as it's faster and often sufficient for
    // flow, or INTEGER if we want exact counts. Existing GurobiSolver uses
    // INTEGER. Let's use CONTINUOUS for speed unless required. Actually,
    // execution counts should be effectively integer. Let's use CONTINUOUS for
    // now.

    Error =
        GRBaddvar(Model, 0, nullptr, nullptr, Obj, Lb, Ub, VType, Name.c_str());
    if (Error)
      break;
  }

  if (Error) {
    errs() << "Error adding node variables\n";
    GRBfreemodel(Model);
    GRBfreeenv(Env);
    return Result;
  }

  // Map Edge (U, V) -> VarIdx
  std::map<std::pair<unsigned, unsigned>, int> EdgeToVarIdx;

  // 2. Add Edge Variables
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    for (const auto &Edge : ASG.getSuccessors(U)) {
      unsigned V = Edge.To;
      EdgeToVarIdx[{U, V}] = VarIdx++;

      std::string Name = "e_" + std::to_string(U) + "_" + std::to_string(V);
      double Obj = 0.0;

      Error = GRBaddvar(Model, 0, nullptr, nullptr, Obj, 0.0, GRB_INFINITY,
                        GRB_CONTINUOUS, Name.c_str());
      if (Error)
        break;
    }
    if (Error)
      break;
  }

  GRBupdatemodel(Model);

  // 3. Constraints

  // Flow Conservation
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    const auto &Node = NodePair.second;

    // x_u = Sum(InEdges)
    if (!Node->IsEntry) {
      std::vector<int> Ind;
      std::vector<double> Val;

      Ind.push_back(NodeToVarIdx[U]);
      Val.push_back(1.0);

      for (unsigned Pred : ASG.getPredecessors(U)) {
        if (EdgeToVarIdx.count({Pred, U})) {
          Ind.push_back(EdgeToVarIdx[{Pred, U}]);
          Val.push_back(-1.0);
        }
      }

      std::string Name = "FlowIn_" + std::to_string(U);
      GRBaddconstr(Model, Ind.size(), Ind.data(), Val.data(), GRB_EQUAL, 0.0,
                   Name.c_str());
    }

    // x_u = Sum(OutEdges)
    if (!Node->IsExit) {
      std::vector<int> Ind;
      std::vector<double> Val;

      Ind.push_back(NodeToVarIdx[U]);
      Val.push_back(1.0);

      for (const auto &Edge : ASG.getSuccessors(U)) {
        unsigned Succ = Edge.To;
        if (EdgeToVarIdx.count({U, Succ})) {
          Ind.push_back(EdgeToVarIdx[{U, Succ}]);
          Val.push_back(-1.0);
        }
      }

      std::string Name = "FlowOut_" + std::to_string(U);
      GRBaddconstr(Model, Ind.size(), Ind.data(), Val.data(), GRB_EQUAL, 0.0,
                   Name.c_str());
    }

    // Entry Constraint
    if (Node->IsEntry) {
      int Idx = NodeToVarIdx[U];
      double Coeff = 1.0;
      GRBaddconstr(Model, 1, &Idx, &Coeff, GRB_EQUAL, 1.0, "Entry");
    }

    // Loop Constraints
    // If Node IS a Header (has IsLoopHeader), and we have a bound.
    // Ideally we assume flow from BackEdge <= Bound * FlowFromOutside.
    // Or x_header <= Bound * FlowFromOutside.
    // FlowFromOutside = x_header - FlowFromBackEdge.
    // So x_header <= Bound * (x_header - FlowFromBackEdge)
    // Bound * FlowBackEdge <= (Bound - 1) * x_header.
    // We need to identify FlowFromBackEdge.
    // We iterate Predecessors. If Pred -> U is a BackEdge.
    if (Node->IsLoopHeader && Node->UpperLoopBound > 0) {
      std::vector<int> Ind;
      std::vector<double> Val;

      // (Bound - 1) * x_header
      Ind.push_back(NodeToVarIdx[U]);
      Val.push_back(static_cast<double>(Node->UpperLoopBound - 1));

      // - Bound * Sum(x_backedge)
      for (unsigned Pred : ASG.getPredecessors(U)) {
        // Find edge from Pred to U and check IsBackEdge
        // Since getPredecessors returns IDs, we need to check Pred's successors
        // to find the Edge struct
        bool IsBack = false;
        for (const auto &Edge : ASG.getSuccessors(Pred)) {
          if (Edge.To == U && Edge.IsBackEdge) {
            IsBack = true;
            break;
          }
        }

        if (IsBack && EdgeToVarIdx.count({Pred, U})) {
          Ind.push_back(EdgeToVarIdx[{Pred, U}]);
          Val.push_back(-static_cast<double>(Node->UpperLoopBound));
        }
      }
      // Constraint: LHS >= 0
      // (Bound-1)*x_h - Bound*x_back >= 0
      // (Bound-1)*x_h >= Bound*x_back.
      // This allows x_h to be larger (multiplier).
      // Correct.
      std::string Name = "LoopBound_" + std::to_string(U);
      GRBaddconstr(Model, Ind.size(), Ind.data(), Val.data(), GRB_GREATER_EQUAL,
                   0.0, Name.c_str());
    }
  }

  // Optimize
  GRBoptimize(Model);

  int Status;
  GRBgetintattr(Model, GRB_INT_ATTR_STATUS, &Status);

  if (Status == GRB_OPTIMAL) {
    GRBgetdblattr(Model, GRB_DBL_ATTR_OBJVAL, &Result.WCET);

    // Get Execution Counts
    // We need to query variables
    for (const auto &Pair : NodeToVarIdx) {
      double X;
      GRBgetdblattrelement(Model, GRB_DBL_ATTR_X, Pair.second, &X);
      if (X > 0.0001) {
        Result.ExecutionCounts[Pair.first] = X;
      }
    }
  } else {
    errs() << "Gurobi optimization failed with status " << Status << "\n";
  }

  GRBfreemodel(Model);
  GRBfreeenv(Env);

#else
  errs() << "Gurobi not enabled.\n";
#endif

  return Result;
}

} // namespace llvm
