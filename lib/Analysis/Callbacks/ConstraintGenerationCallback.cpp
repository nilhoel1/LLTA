/**
 * @file ConstraintGenerationCallback.cpp
 * @brief Implementation of ConstraintGenerationCallback
 */

#include "Analysis/Callbacks/ConstraintGenerationCallback.h"

namespace llvm {

ConstraintGenerationCallback::ConstraintGenerationCallback() {}

void ConstraintGenerationCallback::onNodeAdded(unsigned nodeId,
                                               const MachineBasicBlock *mbb) {
  std::set<unsigned> emptySet;
  nodeIncomingEdges.emplace_back(std::make_pair(nodeId, emptySet));
  nodeOutgoingEdges.emplace_back(std::make_pair(nodeId, emptySet));
}

void ConstraintGenerationCallback::onEdgeAdded(unsigned from, unsigned to,
                                               bool isBackEdge) {
  auto &outgoingSet = nodeOutgoingEdges[from].second;
  auto &incomingSet = nodeIncomingEdges[to].second;

  outgoingSet.insert(to);
  incomingSet.insert(from);

  if (isBackEdge) {
    loopHeaders.insert(to);
  }
}

void ConstraintGenerationCallback::generateFlowConstraint(unsigned nodeId) {
  std::string constraint;

  std::set<unsigned> incomingSet;
  std::set<unsigned> outgoingSet;

  for (const auto &entry : nodeIncomingEdges) {
    if (entry.first == nodeId) {
      incomingSet = entry.second;
      break;
    }
  }

  for (const auto &entry : nodeOutgoingEdges) {
    if (entry.first == nodeId) {
      outgoingSet = entry.second;
      break;
    }
  }

  if (incomingSet.empty() && outgoingSet.empty()) {
    return;
  }

  if (incomingSet.empty()) {
    constraint =
        "flow_conservation_" + std::to_string(nodeId) + ": outgoing_sum = 1";
  } else if (outgoingSet.empty()) {
    constraint =
        "flow_conservation_" + std::to_string(nodeId) + ": incoming_sum = 1";
  } else {
    constraint = "flow_conservation_" + std::to_string(nodeId) +
                 ": incoming_sum = outgoing_sum";
  }

  flowConstraints.push_back(constraint);
}

void ConstraintGenerationCallback::onGraphBuilt() {
  dbgs() << "[ConstraintGen] Generating flow constraints...\n";

  for (const auto &entry : nodeOutgoingEdges) {
    generateFlowConstraint(entry.first);
  }

  dbgs() << "[ConstraintGen] Generated " << flowConstraints.size()
         << " flow constraints\n";

  dbgs() << "[ConstraintGen] Generating loop bound constraints...\n";

  for (unsigned loopHeader : loopHeaders) {
    std::string constraint = "loop_bound_" + std::to_string(loopHeader) +
                             ": backedge_sum <= " + std::to_string(loopHeader);
    loopBoundConstraints.push_back(constraint);
  }

  dbgs() << "[ConstraintGen] Generated " << loopBoundConstraints.size()
         << " loop bound constraints\n";
}

const std::vector<std::string> &
ConstraintGenerationCallback::getFlowConstraints() const {
  return flowConstraints;
}

const std::vector<std::string> &
ConstraintGenerationCallback::getLoopBoundConstraints() const {
  return loopBoundConstraints;
}

std::string ConstraintGenerationCallback::getConstraintSummary() const {
  std::string summary =
      "Total Constraints: " +
      std::to_string(flowConstraints.size() + loopBoundConstraints.size()) +
      "\n";
  summary +=
      "  Flow Constraints: " + std::to_string(flowConstraints.size()) + "\n";
  summary += "  Loop Bound Constraints: " +
             std::to_string(loopBoundConstraints.size()) + "\n";
  return summary;
}

} // namespace llvm
