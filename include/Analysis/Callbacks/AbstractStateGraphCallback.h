#ifndef ABSTRACT_STATE_GRAPH_CALLBACK_H
#define ABSTRACT_STATE_GRAPH_CALLBACK_H

#include "llvm/CodeGen/MachineBasicBlock.h"

namespace llvm {

class AbstractState;

class AbstractStateGraphCallback {
public:
  virtual ~AbstractStateGraphCallback() = default;

  virtual void onNodeAdded(unsigned nodeId, const MachineBasicBlock *mbb) {}
  virtual void onEdgeAdded(unsigned from, unsigned to, bool isBackEdge) {}
  virtual void onStateUpdated(unsigned nodeId, const AbstractState *newState) {}
  virtual void onGraphBuilt() {}

  virtual bool canJoin(unsigned from, unsigned to) { return true; }
};

} // namespace llvm

#endif // ABSTRACT_STATE_GRAPH_CALLBACK_H
