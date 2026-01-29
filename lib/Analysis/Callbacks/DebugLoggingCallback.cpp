/**
 * @file DebugLoggingCallback.cpp
 * @brief Implementation of DebugLoggingCallback
 */

#include "Analysis/Callbacks/DebugLoggingCallback.h"
#include "Analysis/AbstractState.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

DebugLoggingCallback::DebugLoggingCallback(bool logNodes, bool logEdges,
                                           bool logStateUpdates)
    : LogNodes(logNodes), LogEdges(logEdges), LogStateUpdates(logStateUpdates),
      NodeCount(0), EdgeCount(0), StateUpdateCount(0) {}

void DebugLoggingCallback::onNodeAdded(unsigned nodeId,
                                       const MachineBasicBlock *mbb) {
  if (!LogNodes) {
    return;
  }

  if (mbb) {
    outs() << "[ASG] Added Node " << nodeId << " -> MBB: " << mbb->getName()
           << " (" << mbb << ")\n";
  } else {
    outs() << "[ASG] Added Node " << nodeId << " -> MBB: (null)\n";
  }

  NodeCount++;
}

void DebugLoggingCallback::onEdgeAdded(unsigned from, unsigned to,
                                       bool isBackEdge) {
  if (!LogEdges) {
    return;
  }

  outs() << "[ASG] Added Edge " << from << " -> " << to
         << (isBackEdge ? " (BackEdge)" : "") << "\n";

  EdgeCount++;
}

void DebugLoggingCallback::onStateUpdated(unsigned nodeId,
                                          const AbstractState *newState) {
  if (!LogStateUpdates) {
    return;
  }

  outs() << "[ASG] State Updated: " << nodeId << " -> " << newState->toString()
         << "\n";

  StateUpdateCount++;
}

void DebugLoggingCallback::onGraphBuilt() {
  outs() << "[ASG] Graph Built: " << NodeCount << " nodes, " << EdgeCount
         << " edges";

  if (LogStateUpdates && StateUpdateCount > 0) {
    outs() << ", " << StateUpdateCount << " state updates";
  }

  outs() << "\n";
}

} // namespace llvm
