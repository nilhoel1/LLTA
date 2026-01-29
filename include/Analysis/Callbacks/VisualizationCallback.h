#ifndef VISUALIZATION_CALLBACK_H
#define VISUALIZATION_CALLBACK_H

#include "Analysis/Callbacks/AbstractStateGraphCallback.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <sstream>
#include <string>

namespace llvm {

class VisualizationCallback : public AbstractStateGraphCallback {
public:
  VisualizationCallback(const std::string &outputPath = "asg.dot",
                        bool includeWeights = false,
                        bool highlightBackEdges = true);

  void onNodeAdded(unsigned nodeId, const MachineBasicBlock *mbb) override;
  void onEdgeAdded(unsigned from, unsigned to, bool isBackEdge) override;
  void onGraphBuilt() override;

private:
  std::string getNodeLabel(unsigned nodeId, const MachineBasicBlock *mbb);
  std::string getEdgeAttributes(unsigned from, unsigned to, bool isBackEdge);
  void writeDotFile();

  std::string OutputPath;
  bool IncludeWeights;
  bool HighlightBackEdges;
  std::ostringstream dotStream;
};

} // namespace llvm

#endif // VISUALIZATION_CALLBACK_H
