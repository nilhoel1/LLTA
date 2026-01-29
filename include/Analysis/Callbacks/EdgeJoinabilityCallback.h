#ifndef EDGE_JOINABILITY_CALLBACK_H
#define EDGE_JOINABILITY_CALLBACK_H

#include "Analysis/Callbacks/AbstractStateGraphCallback.h"

namespace llvm {

class EdgeJoinabilityCallback : public AbstractStateGraphCallback {
public:
  EdgeJoinabilityCallback(bool allowBackEdges = true,
                          bool allowCallEdges = true,
                          bool allowReturnEdges = true);

  bool canJoin(unsigned from, unsigned to) override;

private:
  bool AllowBackEdges;
  bool AllowCallEdges;
  bool AllowReturnEdges;
};

} // namespace llvm

#endif // EDGE_JOINABILITY_CALLBACK_H
