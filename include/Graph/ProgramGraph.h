#ifndef PROGRAM_GRAPH_H
#define PROGRAM_GRAPH_H

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <cassert>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llta {
class RTTarget;
} // namespace llta

namespace llvm {

class MachineFunction;
class MachineModuleInfo;
class MachineLoopInfo;

struct MuArchState {
  unsigned MinCycles;
  unsigned MaxCycles;
  std::string DebugInfo;

  MuArchState(unsigned Min, unsigned Max, std::string Info = "")
      : MinCycles(Min), MaxCycles(Max), DebugInfo(Info) {}
  virtual ~MuArchState() = default;

  unsigned getUpperBoundCycles() const { return MaxCycles; }
  unsigned getLowerBoundCycles() const { return MinCycles; }
};

class Node {
public:
  explicit Node(unsigned NewId, std::unique_ptr<MuArchState> State);

  Node(const Node &Node);

  ~Node();

  bool operator<(const Node &Node) const;

  unsigned getId() const;

  const std::set<unsigned> getPredecessors() const;

  const std::set<unsigned> getSuccessors() const;

  void addSuccessor(unsigned SuccessorId);

  void addPredecessor(unsigned PrededecessorId);

  bool deleteSuccessor(unsigned SuccessorId);

  bool deletePredecessor(unsigned PrededecessorId);

  bool isPredecessor(unsigned PrededecessorId) const;

  bool isSuccessor(unsigned SuccessorId) const;

  bool isFree() const;

  void setName(StringRef NewName) { Name = NewName; }

  std::string getNodeDescr() const;

  MuArchState &getState() const;

  bool IsLoop = false;

  bool IsNestedLoop = false;

  unsigned int LowerLoopBound;

  unsigned int UpperLoopBound;

  Node *NestedLoopHeader;

  friend std::ostream &operator<<(std::ostream &Stream, Node Node) {
    Stream << "Node ID: " << Node.Id;
    return Stream;
  }

  /**
   * Stores the id of this Node.
   */
  unsigned Id;

  /**
   * Stores the name of this Node, e.g. the name of the MachineBasicBlock
   * it represents.
   */
  StringRef Name;

  /**
   * Stores all ids of succeeding vertices.
   */
  std::set<unsigned> Successors;

  /**
   * Stores all ids of preceding vertices.
   */
  std::set<unsigned> Predecessors;

  /**
   * Stores ids of predecessors that are backedges (for loop headers).
   */
  std::set<unsigned> BackEdgePredecessors;

  /**
   * Stores the architectural state associated with this Node.
   */
  std::unique_ptr<MuArchState> State;
};

class ProgramGraph {

public:
  const bool DebugPrints = false;
  const bool Verbose = true;

  ProgramGraph();

  ProgramGraph(ProgramGraph &G2);

  ~ProgramGraph();

  unsigned addNode(std::unique_ptr<MuArchState> State, MachineBasicBlock *MBB);
  unsigned addNode(std::unique_ptr<MuArchState> State, MachineBasicBlock *MBB,
                   StringRef NodeName);

  /**
   * Adds an edge to the graph from the Node with id start to the Node with
   * id end.
   */
  void addEdge(unsigned FromNode, unsigned ToNode);

  /**
   * Wire the synthetic Entry/Exit nodes of an entry function. Pure with respect
   * to MachineIR: \p BodyNodeIds are the graph nodes for the function's
   * MachineBasicBlocks in layout order and \p ReturnNodeIds the subset that are
   * return blocks. Handles the empty-function (Entry->Exit directly) and
   * no-return-block (fallback: last block -> Exit) cases. Returns true once an
   * Entry->...->Exit path is guaranteed (always, by construction).
   */
  bool wireEntryExit(const std::vector<unsigned> &BodyNodeIds,
                     const std::vector<unsigned> &ReturnNodeIds,
                     unsigned EntryNode, unsigned ExitNode);

  void removeNode(unsigned Node);

  /**
   * Removes the edge from the Node with id fromNode to the Node with id
   * toNode from the graph.
   */
  void removeEdge(unsigned FromNode, unsigned ToNode);

  const std::set<unsigned> getPredecessors(unsigned NodeId) const;

  const std::set<unsigned> getSuccessors(unsigned NodeId) const;

  const std::map<unsigned, Node> &getNodes() const;

  bool isFree(unsigned Node) const;

  bool hasEdge(unsigned FromNode, unsigned ToNode) const;

  void dump() const;

  friend std::ostream &operator<<(std::ostream &Stream, ProgramGraph Graph) {
    for (const auto &Nd : Graph.getNodes()) {
      Stream << Nd.second;
    }
    return Stream;
  }

  bool dump2Dot(StringRef FileName);

  /**
   * Get a list of node IDs that exist in the graph but are not mapped to any
   * MBB.
   */
  std::vector<unsigned> getNodesNotInMBBMap() const;

  /**
   * Fill the MuArchStateGraph with nodes and edges from a MachineFunction.
   */
  bool fillGraphWithFunction(
      MachineFunction &MF, bool IsEntry,
      const std::unordered_map<const MachineBasicBlock *, unsigned int>
          &MBBLatencyMap,
      const std::unordered_map<const MachineBasicBlock *, unsigned int>
          &LoopBoundMap = {},
      MachineLoopInfo *MLI = nullptr,
      const std::set<
          std::pair<const MachineBasicBlock *, const MachineBasicBlock *>>
          &IrreducibleBackEdges = {});

  /**
   * Fill the MuArchStateGraph with all functions from a module.
   */
  void
  fillGraph(MachineModuleInfo *MMI,
            const std::unordered_map<const MachineBasicBlock *, unsigned int>
                &MBBLatencyMap,
            const std::unordered_map<const MachineBasicBlock *, unsigned int>
                &LoopBoundMap = {});

  /**
   * Finalize the graph by adding call and return edges. \p Target (may be null)
   * supplies costs for body-less external calls
   * (RTTarget::getExternalCallCost); calls it cannot cost are recorded in
   * UnsoundExternalCallees.
   */
  bool finalize(MachineFunction &MF, MachineModuleInfo *MMI,
                const llta::RTTarget *Target,
                const std::map<std::string, unsigned> &RecursionBounds = {});

  /// Names of self-recursive functions that were given a `recursion_bound` and
  /// thereby bounded (the callee entry was turned into a bounded loop header).
  const std::set<std::string> &getBoundedRecursionFunctions() const {
    return BoundedRecursionFunctions;
  }

  /// Self-recursive functions reachable on the analysis path that have NO
  /// supplied recursion bound: their call cycle is unbounded and the WCET does
  /// not exist. Reported by finalize().
  const std::set<std::string> &getUnboundedRecursionFunctions() const {
    return UnboundedRecursionFunctions;
  }

  /// Functions found in a multi-function call-graph cycle (mutual recursion).
  /// Bounding these is out of scope; their cycle is unbounded and reported.
  const std::set<std::string> &getMutualRecursionFunctions() const {
    return MutualRecursionFunctions;
  }

  /// Names of reachable external callees whose body is absent from the IR and
  /// for which the target supplied no cost (so their cost was omitted). When
  /// non-empty the WCET is an under-approximation; PathAnalysisPass lists
  /// these.
  const std::set<std::string> &getUnsoundExternalCallees() const {
    return UnsoundExternalCallees;
  }

  /**
   * The set vertices in the graph.
   * Edges are also contained in this set, each edge has two entries,
   * 	one in the preceding, one in the succeeding Node.
   */
  std::map<unsigned, Node> Nodes;

  /**
   * Map that stores a relation from MBB to Nodes
   */
  std::map<const MachineBasicBlock *, unsigned> MBBToNodeMap;

  /**
   * Map from Node id to its parent IR Function. Captured at insertion time
   * because the corresponding MachineBasicBlock may be freed before dump2Dot
   * runs (MachineFunctions are released after their per-function pass chain
   * finishes), but IR Function pointers are stable for the module's lifetime.
   */
  std::map<unsigned, const Function *> NodeToFunctionMap;

  /**
   * Whether a synthetic Entry node has been created (i.e. a start function was
   * seen). Used as the root for reachability pruning in finalize().
   */
  bool HasEntryNode = false;

  /**
   * Id of the synthetic Entry node created for the start function.
   */
  unsigned EntryNodeId = 0;

  /**
   * The start function of the analysis (the one given the synthetic Entry/Exit
   * nodes). It is the analysis boundary: entered only via Entry and left only
   * via Exit, so call sites that call into it (external callers) must not be
   * wired during finalize().
   */
  const Function *StartFunction = nullptr;

  /**
   * Map that stores the entry node for each Function.
   */
  std::map<const Function *, unsigned> FunctionToEntryNodeMap;

  /**
   * Map that stores the return nodes for each Function.
   */
  std::map<const Function *, std::vector<unsigned>> FunctionToReturnNodesMap;

  /**
   * A captured call site: the caller's node, the callee Function, and the node
   * of the return-landing block (the caller block's continuation successor,
   * which CallSplitterPass placed immediately after the call). HasLanding is
   * false when the call has no continuation successor (e.g. a noreturn or tail
   * call), in which case finalize() wires no return edge.
   */
  struct CallSite {
    unsigned CallNode;
    const Function *Callee;
    unsigned LandingNode;
    bool HasLanding;
  };

  /**
   * Store call sites for wiring call and return edges in finalize().
   */
  std::vector<CallSite> CallSites;

  /**
   * Call sites to backend-synthesized libcalls (MO_ExternalSymbol, e.g.
   * __mspabi_*): caller node -> callee symbol name. These have no IR Function,
   * so they are resolved by name against the target's external-call cost model
   * in finalize().
   */
  std::vector<std::pair<unsigned, std::string>> ExternalSymbolCallSites;

  /**
   * Render info for body-less external/ABI calls (e.g. \c __mspabi_*), filled by
   * finalize(): the calling node, the callee symbol, and the cycle cost charged
   * to the caller. \c Costed is false when no cost model existed (the call is
   * then unsound). These calls have no block of their own --- their cost is
   * folded into the caller --- so dump2Dot uses this to draw them explicitly.
   */
  struct ExternalCallRender {
    unsigned CallNode;
    std::string Name;
    unsigned Cost;
    bool Costed;
  };
  std::vector<ExternalCallRender> ExternalCallRenders;

  /**
   * Reachable body-less external callees that could not be costed (see
   * getUnsoundExternalCallees).
   */
  std::set<std::string> UnsoundExternalCallees;

  /// Recursion diagnostics filled by finalize() (see the getters above).
  std::set<std::string> BoundedRecursionFunctions;
  std::set<std::string> UnboundedRecursionFunctions;
  std::set<std::string> MutualRecursionFunctions;

private:
  /**
   * A counter to give unique identifiers to each Node.
   */
  unsigned NextNodeId;
};

} // end namespace llvm

#endif // PROGRAM_GRAPH_H
