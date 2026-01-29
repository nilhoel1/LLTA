/**
 * @file EdgeJoinabilityCallback.cpp
 * @brief Implementation of EdgeJoinabilityCallback
 */

#include "Analysis/Callbacks/EdgeJoinabilityCallback.h"

namespace llvm {

EdgeJoinabilityCallback::EdgeJoinabilityCallback(bool allowBackEdges,
                                                 bool allowCallEdges,
                                                 bool allowReturnEdges)
    : AllowBackEdges(allowBackEdges), AllowCallEdges(allowCallEdges),
      AllowReturnEdges(allowReturnEdges) {}

bool EdgeJoinabilityCallback::canJoin(unsigned from, unsigned to) {
  return AllowBackEdges && AllowCallEdges && AllowReturnEdges;
}

} // namespace llvm
