/**
 * @file VisualizationCallback.cpp
 * @brief Implementation of VisualizationCallback
 */

#include "Analysis/Callbacks/VisualizationCallback.h"
#include <fstream>

namespace llvm {

VisualizationCallback::VisualizationCallback(const std::string &outputPath,
                                             bool includeWeights,
                                             bool highlightBackEdges)
    : OutputPath(outputPath), IncludeWeights(includeWeights),
      HighlightBackEdges(highlightBackEdges) {
  dotStream << "digraph AbstractStateGraph {\n";
}

void VisualizationCallback::onNodeAdded(unsigned nodeId,
                                        const MachineBasicBlock *mbb) {
  dotStream << "  " << nodeId << " [" << getNodeLabel(nodeId, mbb) << "];\n";
}

void VisualizationCallback::onEdgeAdded(unsigned from, unsigned to,
                                        bool isBackEdge) {
  dotStream << "  " << from << " -> " << to << " "
            << getEdgeAttributes(from, to, isBackEdge) << ";\n";
}

void VisualizationCallback::onGraphBuilt() {
  dotStream << "}\n";
  writeDotFile();
}

std::string VisualizationCallback::getNodeLabel(unsigned nodeId,
                                                const MachineBasicBlock *mbb) {
  std::ostringstream oss;
  oss << "label=\"" << nodeId;

  if (mbb) {
    oss << ": " << mbb->getName().str() << "\"";
  } else {
    oss << "\"";
  }

  return oss.str();
}

std::string VisualizationCallback::getEdgeAttributes(unsigned from, unsigned to,
                                                     bool isBackEdge) {
  std::ostringstream oss;

  if (isBackEdge && HighlightBackEdges) {
    oss << ", color=red, style=dashed";
  }

  if (IncludeWeights) {
    oss << ", label=\"" << from << "\"";
  }

  return oss.str();
}

void VisualizationCallback::writeDotFile() {
  std::ofstream file(OutputPath);

  if (!file.is_open()) {
    errs() << "[VisualizationCallback] Error opening file: " << OutputPath
           << "\n";
    return;
  }

  file << dotStream.str();
  file.close();

  outs() << "[VisualizationCallback] Wrote DOT graph to: " << OutputPath
         << "\n";
}

} // namespace llvm
