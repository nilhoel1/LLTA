#ifndef GRAPH_ADAPTER_H
#define GRAPH_ADAPTER_H

#include "Analysis/AbstractStateGraph.h"
#include "RTTargets/ProgramGraph.h"

namespace llvm {

/**
 * Adapter to convert AbstractStateGraph to ProgramGraph.
 * This allows reusing the existing ILP solver infrastructure.
 */
class GraphAdapter {
public:
  /**
   * Convert AbstractStateGraph to ProgramGraph.
   * NewNodesMap will be populated with a mapping from AbstractStateGraph Node
   * ID to ProgramGraph Node ID.
   */
  static ProgramGraph convert(AbstractStateGraph &ASG,
                              std::map<unsigned, unsigned> &NewNodesMap);
};

} // namespace llvm

#endif // GRAPH_ADAPTER_H
