#include "ILP/HighsSolver.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#ifdef ENABLE_HIGHS
#include <Highs.h>
#endif

#define DEBUG_TYPE "ilp"

namespace llvm {

HighsSolver::HighsSolver() = default;

HighsSolver::~HighsSolver() = default;

bool HighsSolver::isAvailable() const {
#ifdef ENABLE_HIGHS
  return true;
#else
  return false;
#endif
}

ILPResult
HighsSolver::solveWCET(const ProgramGraph &MASG, unsigned EntryNodeId,
                       unsigned ExitNodeId,
                       const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;

#ifdef ENABLE_HIGHS
  Highs Highs;
  Highs.setOptionValue("output_flag", false);

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

  int TotalVars = NumNodes + NumEdges;

  // Build the ILP model
  HighsModel Model;
  Model.lp_.num_col_ = TotalVars;
  Model.lp_.num_row_ = 0; // Will be set after adding constraints
  Model.lp_.sense_ = ObjSense::kMaximize;
  Model.lp_.offset_ = 0;

  // Set column (variable) bounds and costs
  Model.lp_.col_cost_.resize(TotalVars);
  Model.lp_.col_lower_.resize(TotalVars, 0.0);
  Model.lp_.col_upper_.resize(TotalVars, kHighsInf);
  Model.lp_.integrality_.resize(TotalVars, HighsVarType::kInteger);

  // Set costs for node variables
  for (const auto &NodePair : Nodes) {
    int Idx = NodeToVarIdx[NodePair.first];
    const Node &N = NodePair.second;
    Model.lp_.col_cost_[Idx] = N.getState().getUpperBoundCycles();
  }

  // Set costs for edge variables (0)
  for (int I = 0; I < NumEdges; ++I) {
    Model.lp_.col_cost_[NumNodes + I] = 0.0;
  }

  // Prepare constraint storage
  std::vector<double> RowLower;
  std::vector<double> RowUpper;
  std::vector<HighsInt> AStart;
  std::vector<HighsInt> AIndex;
  std::vector<double> AValue;

  int CurrentNnz = 0;

  // Constraint 1: NrTakenNode_entry = 1
  {
    AStart.push_back(CurrentNnz);
    AIndex.push_back(NodeToVarIdx[EntryNodeId]);
    AValue.push_back(1.0);
    CurrentNnz++;
    RowLower.push_back(1.0);
    RowUpper.push_back(1.0);
  }

  // Constraint 2: NrTakenNode_exit = 1
  {
    AStart.push_back(CurrentNnz);
    AIndex.push_back(NodeToVarIdx[ExitNodeId]);
    AValue.push_back(1.0);
    CurrentNnz++;
    RowLower.push_back(1.0);
    RowUpper.push_back(1.0);
  }

  // Constraint 3: Flow conservation with edge variables
  // For each node i: sum(incoming edges) = x_i and sum(outgoing edges) = x_i
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    const auto &Preds = N.getPredecessors();
    const auto &Succs = N.getSuccessors();

    // Incoming flow conservation: sum(e_{pred,i}) = x_i
    // Rearranged: sum(e_{pred,i}) - x_i = 0
    if (!Preds.empty()) {
      AStart.push_back(CurrentNnz);

      // Add node variable x_i with coefficient -1
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(-1.0);
      CurrentNnz++;

      // Add incoming edge variables with coefficient +1
      for (unsigned PredId : Preds) {
        auto EdgeKey = std::make_pair(PredId, NodeId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          AIndex.push_back(EdgeToVarIdx[EdgeKey]);
          AValue.push_back(1.0);
          CurrentNnz++;
        }
      }
      RowLower.push_back(0.0);
      RowUpper.push_back(0.0);
    }

    // Outgoing flow conservation: sum(e_{i,succ}) = x_i
    // Rearranged: sum(e_{i,succ}) - x_i = 0
    if (!Succs.empty()) {
      AStart.push_back(CurrentNnz);

      // Add node variable x_i with coefficient -1
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(-1.0);
      CurrentNnz++;

      // Add outgoing edge variables with coefficient +1
      for (unsigned SuccId : Succs) {
        auto EdgeKey = std::make_pair(NodeId, SuccId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          AIndex.push_back(EdgeToVarIdx[EdgeKey]);
          AValue.push_back(1.0);
          CurrentNnz++;
        }
      }
      RowLower.push_back(0.0);
      RowUpper.push_back(0.0);
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

    // Identify back edges vs entry edges using explicit backedge info
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
    if (BackEdgePreds.empty() && !N.getPredecessors().empty()) {
      PreheaderPreds.clear();
      for (unsigned PredId : N.getPredecessors()) {
        bool IsBackEdge = (PredId > NodeId);
        if (IsBackEdge) {
          BackEdgePreds.push_back(PredId);
        } else {
          PreheaderPreds.push_back(PredId);
        }
      }
      if (BackEdgePreds.empty()) {
        PreheaderPreds.clear();
        for (unsigned PredId : N.getPredecessors()) {
          PreheaderPreds.push_back(PredId);
        }
      }
    }

    // Constraint: x_header <= LoopBound * sum(preheader_flow)
    // Rearranged: x_header - LoopBound * sum(flow(preheader -> header)) <= 0
    if (!PreheaderPreds.empty()) {
      AStart.push_back(CurrentNnz);
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(1.0);
      CurrentNnz++;

      for (unsigned PrehId : PreheaderPreds) {
        // Use edge variable for flow from preheader to header
        auto EdgeKey = std::make_pair(PrehId, NodeId);
        if (EdgeToVarIdx.find(EdgeKey) != EdgeToVarIdx.end()) {
          AIndex.push_back(EdgeToVarIdx[EdgeKey]);
          AValue.push_back(-static_cast<double>(LoopBound));
          CurrentNnz++;
        } else {
          // Fallback to node variable if edge not found (should not happen)
          AIndex.push_back(NodeToVarIdx[PrehId]);
          AValue.push_back(-static_cast<double>(LoopBound));
          CurrentNnz++;
        }
      }

      RowLower.push_back(-kHighsInf);
      RowUpper.push_back(0.0);
    } else {
      // No preheader found, use absolute bound
      AStart.push_back(CurrentNnz);
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(1.0);
      CurrentNnz++;

      RowLower.push_back(-kHighsInf);
      RowUpper.push_back(static_cast<double>(LoopBound));
    }
  }

  // Finalize the constraint matrix
  AStart.push_back(CurrentNnz);

  Model.lp_.num_row_ = RowLower.size();
  Model.lp_.row_lower_ = RowLower;
  Model.lp_.row_upper_ = RowUpper;

  // HiGHS uses column-wise sparse matrix, but we built row-wise
  // Convert to column-wise format
  Model.lp_.a_matrix_.format_ = MatrixFormat::kRowwise;
  Model.lp_.a_matrix_.start_ = AStart;
  Model.lp_.a_matrix_.index_ = AIndex;
  Model.lp_.a_matrix_.value_ = AValue;

  // Pass model to HiGHS
  HighsStatus Status = Highs.passModel(Model);
  if (Status != HighsStatus::kOk) {
    Result.StatusMessage = "Failed to pass model to HiGHS";
    return Result;
  }

  // Write model to file for debugging
  Highs.writeModel("highs_wcet_model.mps");

  // Also write a human-readable LP format
  std::error_code EC;
  llvm::raw_fd_ostream LPFile("highs_wcet_model.lp", EC);
  if (!EC) {
    auto GetVarName = [&](int Idx) -> std::string {
      if (Idx < NumNodes) {
        return "N" + std::to_string(VarIdxToNode[Idx]);
      }
      int EdgeIdx = Idx - NumNodes;
      // We don't have a direct mapping from EdgeIdx to a simple ID,
      // but we can use E + index
      return "E" + std::to_string(EdgeIdx);
    };

    LPFile << "\\* WCET ILP Model (HiGHS) *\\\n";
    LPFile << "Maximize\n obj: ";
    for (int I = 0; I < TotalVars; I++) {
      if (Model.lp_.col_cost_[I] != 0) {
        if (I > 0 && Model.lp_.col_cost_[I] > 0)
          LPFile << " + ";
        LPFile << Model.lp_.col_cost_[I] << " " << GetVarName(I);
      }
    }
    LPFile << "\n\nSubject To\n";

    // Print constraints
    for (size_t Row = 0; Row < RowLower.size(); Row++) {
      LPFile << " c" << Row << ": ";
      int Start = AStart[Row];
      int End = AStart[Row + 1];
      for (int Nz = Start; Nz < End; Nz++) {
        if (Nz > Start && AValue[Nz] > 0)
          LPFile << " + ";
        LPFile << AValue[Nz] << " " << GetVarName(AIndex[Nz]);
      }
      if (RowLower[Row] == RowUpper[Row]) {
        LPFile << " = " << RowLower[Row];
      } else {
        if (RowLower[Row] > -kHighsInf) {
          LPFile << " >= " << RowLower[Row];
        }
        if (RowUpper[Row] < kHighsInf) {
          if (RowLower[Row] > -kHighsInf)
            LPFile << ",";
          LPFile << " <= " << RowUpper[Row];
        }
      }
      LPFile << "\n";
    }

    LPFile << "\nBounds\n";
    for (int I = 0; I < TotalVars; I++) {
      LPFile << " 0 <= " << GetVarName(I) << " <= +inf\n";
    }

    LPFile << "\nInteger\n";
    for (int I = 0; I < TotalVars; I++) {
      LPFile << " " << GetVarName(I) << "\n";
    }
    LPFile << "End\n";
    LPFile.close();
  }

  // Solve
  Status = Highs.run();
  if (Status != HighsStatus::kOk) {
    Result.StatusMessage = "HiGHS optimization failed";
    return Result;
  }

  // Get solution info
  const HighsInfo &Info = Highs.getInfo();
  HighsModelStatus ModelStatus = Highs.getModelStatus();

  if (ModelStatus == HighsModelStatus::kOptimal) {
    Result.Success = true;
    Result.ObjectiveValue = Info.objective_function_value;
    Result.StatusMessage = "Optimal solution found";

    // Get variable values
    const std::vector<double> &Sol = Highs.getSolution().col_value;

    // Extract node execution counts
    for (int I = 0; I < NumNodes; I++) {
      Result.NodeExecutionCounts[VarIdxToNode[I]] = Sol[I];
    }

    // Extract edge execution counts
    for (int I = 0; I < NumEdges; I++) {
      Result.EdgeExecutionCounts[VarIdxToEdge[I]] = Sol[NumNodes + I];
    }
  } else if (ModelStatus == HighsModelStatus::kInfeasible) {
    Result.StatusMessage = "Model is infeasible";
  } else if (ModelStatus == HighsModelStatus::kUnbounded) {
    Result.StatusMessage = "Model is unbounded";
  } else {
    Result.StatusMessage = "Optimization ended with status " +
                           std::to_string(static_cast<int>(ModelStatus));
  }

#else
  Result.StatusMessage = "HiGHS support not compiled in";
#endif

  return Result;
}

} // namespace llvm
