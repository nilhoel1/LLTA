#include "ILP/AbstractHighsSolver.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#ifdef ENABLE_HIGHS
#include "Highs.h"
#endif

#define DEBUG_TYPE "abstract-highs-solver"

namespace llvm {

AbstractHighsSolver::AbstractHighsSolver() {}

AbstractHighsSolver::~AbstractHighsSolver() {}

AbstractILPResult
AbstractHighsSolver::solveWCET(const AbstractStateGraph &ASG) {
  AbstractILPResult Result;
  Result.WCET = 0.0;

#ifdef ENABLE_HIGHS
  Highs highs;
  highs.setOptionValue("output_flag", false);

  HighsModel model;
  model.lp_.sense_ = ObjSense::kMaximize;

  // Map NodeID -> ColIndex (Execution Count)
  std::map<unsigned, int> NodeCols;

  // Map Edge (From, To) -> ColIndex (Edge Flow)
  std::map<std::pair<unsigned, unsigned>, int> EdgeCols;

  // 1. Create Variables (Columns) and Objective
  // In HiGHS, it's often easier to define columns first.

  // Node Variables
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    const auto &Node = NodePair.second;

    int colIdx = model.lp_.num_col_;
    model.lp_.col_cost_.push_back(Node->Cost);
    model.lp_.col_lower_.push_back(0.0);
    model.lp_.col_upper_.push_back(kHighsInf);
    NodeCols[U] = colIdx;
    model.lp_.num_col_++;
  }

  // Edge Variables
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    for (const auto &Edge : ASG.getSuccessors(U)) {
      unsigned V = Edge.To;
      int colIdx = model.lp_.num_col_;
      model.lp_.col_cost_.push_back(0.0);
      model.lp_.col_lower_.push_back(0.0);
      model.lp_.col_upper_.push_back(kHighsInf);
      EdgeCols[{U, V}] = colIdx;
      model.lp_.num_col_++;
    }
  }

  // 2. Constraints (Rows)
  // We use HighsLp::a_matrix_ which is Compressed Sparse Column (CSC) format.
  // But constructing it manually IS PAINFUL.
  // Easier way: Build std::vector<HighsInt> index, ... and pass to addRow?
  // But we are building `model.lp_` directly.
  // Actually, let's use `highs.addVar` and `highs.addRow` interface if
  // possible? But standard usage in LLVM project usually builds the model.
  // Let's assume we can build the matrix row-by-row but that requires CSR
  // (Compressed Sparse Row). HighsLp uses CSC by default (`a_matrix_.format_`
  // is `MatrixFormat::kColwise`). BUT we can use `highs.passModel(model)`
  // later. If we want to add rows easily, maybe we should use `highs.addRow`.
  // Let's pass the columns first, then add rows.

  highs.passModel(model);

  // Flow Conservation
  for (const auto &NodePair : ASG.getNodes()) {
    unsigned U = NodePair.first;
    const auto &Node = NodePair.second;

    // x_u = Sum(InEdges) => x_u - Sum(InEdges) = 0
    if (!Node->IsEntry) {
      std::vector<int> inds;
      std::vector<double> vals;

      inds.push_back(NodeCols[U]);
      vals.push_back(1.0);

      for (unsigned Pred : ASG.getPredecessors(U)) {
        // We need the edge variable index from Pred to U
        if (EdgeCols.count({Pred, U})) {
          inds.push_back(EdgeCols[{Pred, U}]);
          vals.push_back(-1.0);
        }
      }
      highs.addRow(0.0, 0.0, inds.size(), inds.data(), vals.data());
    }

    // x_u = Sum(OutEdges) => x_u - Sum(OutEdges) = 0
    if (!Node->IsExit) {
      std::vector<int> inds;
      std::vector<double> vals;

      inds.push_back(NodeCols[U]);
      vals.push_back(1.0);

      for (const auto &Edge : ASG.getSuccessors(U)) {
        unsigned Succ = Edge.To;
        if (EdgeCols.count({U, Succ})) {
          inds.push_back(EdgeCols[{U, Succ}]);
          vals.push_back(-1.0);
        }
      }
      highs.addRow(0.0, 0.0, inds.size(), inds.data(), vals.data());
    }

    // Entry Constraint: x_entry = 1
    if (Node->IsEntry) {
      int idx = NodeCols[U];
      double val = 1.0;
      highs.addRow(1.0, 1.0, 1, &idx, &val);
    }

    // Loop Constraints
    if (Node->IsLoopHeader && Node->UpperLoopBound > 0) {
      // (Bound - 1) * x_h - Bound * Sum(BackEdges) >= 0
      std::vector<int> inds;
      std::vector<double> vals;

      inds.push_back(NodeCols[U]);
      vals.push_back(static_cast<double>(Node->UpperLoopBound - 1));

      for (unsigned Pred : ASG.getPredecessors(U)) {
        bool IsBack = false;
        for (const auto &Edge : ASG.getSuccessors(Pred)) {
          if (Edge.To == U && Edge.IsBackEdge) {
            IsBack = true;
            break;
          }
        }
        if (IsBack && EdgeCols.count({Pred, U})) {
          inds.push_back(EdgeCols[{Pred, U}]);
          vals.push_back(-static_cast<double>(Node->UpperLoopBound));
        }
      }
      highs.addRow(0.0, kHighsInf, inds.size(), inds.data(), vals.data());
    }
  }

  // Solve
  highs.run();

  if (highs.getModelStatus() == HighsModelStatus::kOptimal) {
    Result.WCET = highs.getObjectiveValue();
  }

#else
  errs() << "HiGHS not enabled. Please reconfigure with -DENABLE_HIGHS=ON\n";
#endif

  return Result;
}

} // namespace llvm
