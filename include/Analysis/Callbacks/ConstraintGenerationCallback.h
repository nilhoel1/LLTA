#ifndef CONSTRAINT_GENERATION_CALLBACK_H
#define CONSTRAINT_GENERATION_CALLBACK_H

#include "Analysis/Callbacks/AbstractStateGraphCallback.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace llvm {

class ConstraintGenerationCallback : public AbstractStateGraphCallback {
public:
  ConstraintGenerationCallback();

  void onNodeAdded(unsigned nodeId, const MachineBasicBlock *mbb) override;
  void onEdgeAdded(unsigned from, unsigned to, bool isBackEdge) override;
  void onGraphBuilt() override;

  const std::vector<std::string> &getFlowConstraints() const;
  const std::vector<std::string> &getLoopBoundConstraints() const;
  std::string getConstraintSummary() const;

private:
  void generateFlowConstraint(unsigned nodeId);

  std::vector<std::pair<unsigned, std::set<unsigned>>> nodeIncomingEdges;
  std::vector<std::pair<unsigned, std::set<unsigned>>> nodeOutgoingEdges;
  std::set<unsigned> loopHeaders;
  std::vector<std::string> flowConstraints;
  std::vector<std::string> loopBoundConstraints;
};

} // namespace llvm

#endif // CONSTRAINT_GENERATION_CALLBACK_H
