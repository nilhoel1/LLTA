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
HighsSolver::solveWCET(const MuArchStateGraph &MASG, unsigned EntryNodeId,
                       unsigned ExitNodeId,
                       const std::map<unsigned, unsigned> &LoopBoundMap) {
  ILPResult Result;

#ifdef ENABLE_HIGHS
  Highs highs;
  highs.setOptionValue("output_flag", false);

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

  // Build the ILP model
  HighsModel Model;
  Model.lp_.num_col_ = NumNodes;
  Model.lp_.num_row_ = 0; // Will be set after adding constraints
  Model.lp_.sense_ = ObjSense::kMaximize;
  Model.lp_.offset_ = 0;

  // Set column (variable) bounds and costs
  Model.lp_.col_cost_.resize(NumNodes);
  Model.lp_.col_lower_.resize(NumNodes, 0.0);
  Model.lp_.col_upper_.resize(NumNodes, kHighsInf);
  Model.lp_.integrality_.resize(NumNodes, HighsVarType::kInteger);

  for (const auto &NodePair : Nodes) {
    int Idx = NodeToVarIdx[NodePair.first];
    const Node &N = NodePair.second;
    Model.lp_.col_cost_[Idx] = N.getState().getUpperBoundCycles();
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

  // Constraint 3: Flow conservation for all nodes (except entry and exit)
  for (const auto &NodePair : Nodes) {
    unsigned NodeId = NodePair.first;
    const Node &N = NodePair.second;

    // Skip entry and exit nodes
    if (NodeId == EntryNodeId || NodeId == ExitNodeId) {
      continue;
    }

    // In-flow constraint: sum(predecessors) = x_i
    // sum(predecessors) - x_i = 0
    const auto &Preds = N.getPredecessors();
    if (!Preds.empty()) {
      AStart.push_back(CurrentNnz);
      for (unsigned PredId : Preds) {
        AIndex.push_back(NodeToVarIdx[PredId]);
        AValue.push_back(1.0);
        CurrentNnz++;
      }
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(-1.0);
      CurrentNnz++;
      RowLower.push_back(0.0);
      RowUpper.push_back(0.0);
    }

    // Out-flow constraint: x_i = sum(successors)
    // x_i - sum(successors) = 0
    const auto &Succs = N.getSuccessors();
    if (!Succs.empty()) {
      AStart.push_back(CurrentNnz);
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(1.0);
      CurrentNnz++;
      for (unsigned SuccId : Succs) {
        AIndex.push_back(NodeToVarIdx[SuccId]);
        AValue.push_back(-1.0);
        CurrentNnz++;
      }
      RowLower.push_back(0.0);
      RowUpper.push_back(0.0);
    }
  }

  // Constraint 4: Loop bound constraints
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
    std::vector<unsigned> PreheaderPreds;
    std::vector<unsigned> BackEdgePreds;

    for (unsigned PredId : N.getPredecessors()) {
      // Simple heuristic: if pred has a higher node ID, assume it's from within
      // the loop (back edge)
      if (PredId > NodeId) {
        BackEdgePreds.push_back(PredId);
      } else {
        PreheaderPreds.push_back(PredId);
      }
    }

    // Constraint: x_header <= LoopBound * sum(preheader_flow)
    // Rearranged: x_header - LoopBound * sum(x_preheader) <= 0
    if (!PreheaderPreds.empty()) {
      AStart.push_back(CurrentNnz);
      AIndex.push_back(NodeToVarIdx[NodeId]);
      AValue.push_back(1.0);
      CurrentNnz++;

      for (unsigned PrehId : PreheaderPreds) {
        AIndex.push_back(NodeToVarIdx[PrehId]);
        AValue.push_back(-static_cast<double>(LoopBound));
        CurrentNnz++;
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
  HighsStatus status = highs.passModel(Model);
  if (status != HighsStatus::kOk) {
    Result.StatusMessage = "Failed to pass model to HiGHS";
    return Result;
  }

  // Solve
  status = highs.run();
  if (status != HighsStatus::kOk) {
    Result.StatusMessage = "HiGHS optimization failed";
    return Result;
  }

  // Get solution info
  const HighsInfo &info = highs.getInfo();
  HighsModelStatus modelStatus = highs.getModelStatus();

  if (modelStatus == HighsModelStatus::kOptimal) {
    Result.Success = true;
    Result.ObjectiveValue = info.objective_function_value;
    Result.StatusMessage = "Optimal solution found";

    // Get variable values
    const std::vector<double> &Sol = highs.getSolution().col_value;
    for (int i = 0; i < NumNodes; i++) {
      Result.NodeExecutionCounts[VarIdxToNode[i]] = Sol[i];
    }
  } else if (modelStatus == HighsModelStatus::kInfeasible) {
    Result.StatusMessage = "Model is infeasible";
  } else if (modelStatus == HighsModelStatus::kUnbounded) {
    Result.StatusMessage = "Model is unbounded";
  } else {
    Result.StatusMessage =
        "Optimization ended with status " + std::to_string(static_cast<int>(modelStatus));
  }

#else
  Result.StatusMessage = "HiGHS support not compiled in";
#endif

  return Result;
}

} // namespace llvm
