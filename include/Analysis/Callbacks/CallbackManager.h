#ifndef CALLBACK_MANAGER_H
#define CALLBACK_MANAGER_H

#include "Analysis/Callbacks/AbstractStateGraphCallback.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <memory>
#include <vector>

namespace llvm {

class AbstractState;
class CallbackManager {
public:
  CallbackManager();
  ~CallbackManager();

  void registerCallback(std::unique_ptr<AbstractStateGraphCallback> cb);
  void registerCallback(AbstractStateGraphCallback *cb);
  void unregisterCallback(AbstractStateGraphCallback *cb);

  void notifyNodeAdded(unsigned nodeId, const MachineBasicBlock *mbb);
  void notifyEdgeAdded(unsigned from, unsigned to, bool isBackEdge);
  void notifyStateUpdated(unsigned nodeId, const AbstractState *newState);
  void notifyGraphBuilt();

  bool canJoinEdge(unsigned from, unsigned to) const;

  size_t getCallbackCount() const;
  void dumpCallbackTypes() const;

private:
  std::vector<AbstractStateGraphCallback *> allCallbacks;
  std::vector<std::unique_ptr<AbstractStateGraphCallback>> ownedCallbacks;
};

} // namespace llvm

#endif // CALLBACK_MANAGER_H
