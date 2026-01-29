/**
 * @file CallbackManager.cpp
 * @brief Implementation of CallbackManager
 */

#include "Analysis/Callbacks/CallbackManager.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

namespace llvm {

CallbackManager::CallbackManager() {}

CallbackManager::~CallbackManager() {
  allCallbacks.clear();
  ownedCallbacks.clear();
}

void CallbackManager::registerCallback(
    std::unique_ptr<AbstractStateGraphCallback> cb) {
  if (!cb) {
    return;
  }

  AbstractStateGraphCallback *rawPtr = cb.get();
  ownedCallbacks.push_back(std::move(cb));
  allCallbacks.push_back(rawPtr);
}

void CallbackManager::registerCallback(AbstractStateGraphCallback *cb) {
  if (!cb) {
    return;
  }

  allCallbacks.push_back(cb);
}

void CallbackManager::unregisterCallback(AbstractStateGraphCallback *cb) {
  if (!cb) {
    return;
  }

  auto it = std::remove(allCallbacks.begin(), allCallbacks.end(), cb);
  if (it != allCallbacks.end()) {
    allCallbacks.erase(it, allCallbacks.end());
  }
}

void CallbackManager::notifyNodeAdded(unsigned nodeId,
                                      const MachineBasicBlock *mbb) {
  for (auto *cb : allCallbacks) {
    cb->onNodeAdded(nodeId, mbb);
  }
}

void CallbackManager::notifyEdgeAdded(unsigned from, unsigned to,
                                      bool isBackEdge) {
  for (auto *cb : allCallbacks) {
    cb->onEdgeAdded(from, to, isBackEdge);
  }
}

void CallbackManager::notifyStateUpdated(unsigned nodeId,
                                         const AbstractState *newState) {
  for (auto *cb : allCallbacks) {
    cb->onStateUpdated(nodeId, newState);
  }
}

void CallbackManager::notifyGraphBuilt() {
  for (auto *cb : allCallbacks) {
    cb->onGraphBuilt();
  }
}

bool CallbackManager::canJoinEdge(unsigned from, unsigned to) const {
  if (allCallbacks.empty()) {
    return true;
  }

  for (auto *cb : allCallbacks) {
    if (!cb->canJoin(from, to)) {
      return false;
    }
  }

  return true;
}

size_t CallbackManager::getCallbackCount() const { return allCallbacks.size(); }

void CallbackManager::dumpCallbackTypes() const {
  if (allCallbacks.empty()) {
    dbgs() << "[CallbackManager] No callbacks registered\n";
    return;
  }

  dbgs() << "[CallbackManager] Registered callbacks (" << allCallbacks.size()
         << " total):\n";

  unsigned idx = 0;
  for (const auto *cb : allCallbacks) {
    const std::type_info &info = typeid(*cb);
    dbgs() << "  [" << idx++ << "] " << info.name() << "\n";
  }
}

} // namespace llvm
