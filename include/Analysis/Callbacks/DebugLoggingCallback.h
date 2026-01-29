#ifndef DEBUG_LOGGING_CALLBACK_H
#define DEBUG_LOGGING_CALLBACK_H

#include "Analysis/Callbacks/AbstractStateGraphCallback.h"
#include "llvm/CodeGen/MachineBasicBlock.h"

namespace llvm {

class DebugLoggingCallback : public AbstractStateGraphCallback {
public:
  DebugLoggingCallback(bool logNodes = true, bool logEdges = true,
                       bool logStateUpdates = false);

  void onNodeAdded(unsigned nodeId, const MachineBasicBlock *mbb) override;
  void onEdgeAdded(unsigned from, unsigned to, bool isBackEdge) override;
  void onStateUpdated(unsigned nodeId, const AbstractState *newState) override;
  void onGraphBuilt() override;

private:
  bool LogNodes;
  bool LogEdges;
  bool LogStateUpdates;
  unsigned NodeCount;
  unsigned EdgeCount;
  unsigned StateUpdateCount;
};

} // namespace llvm

#endif // DEBUG_LOGGING_CALLBACK_H
