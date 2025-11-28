#ifndef MU_ARCH_STATE_GRAPH_H
#define MU_ARCH_STATE_GRAPH_H

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <cassert>
#include <map>
#include <ostream>
#include <set>
#include <unordered_map>

namespace llvm {

class MachineFunction;
class MachineModuleInfo;

  struct MuArchState {
    unsigned UpperBoundCycles;
    unsigned LowerBoundCycles;

    MuArchState(unsigned UpperBound, unsigned LowerBound)
        : UpperBoundCycles(UpperBound), LowerBoundCycles(LowerBound) {}

    unsigned getUpperBoundCycles() const { return UpperBoundCycles; }
    unsigned getLowerBoundCycles() const { return LowerBoundCycles; }

    void setUpperBoundCycles(unsigned Cycles) { UpperBoundCycles = Cycles; }
    void setLowerBoundCycles(unsigned Cycles) { LowerBoundCycles = Cycles; }
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

  Node* NestedLoopHeader;

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
   * Stores the architectural state associated with this Node.
   */
  std::unique_ptr<MuArchState> State;
};

class MuArchStateGraph {

public:
  const bool DebugPrints = false;
  const bool Verbose = true;

  MuArchStateGraph();

  MuArchStateGraph(MuArchStateGraph &G2);

  ~MuArchStateGraph();

  unsigned addNode(MuArchState State, MachineBasicBlock * MBB);
  unsigned addNode(MuArchState State, MachineBasicBlock * MBB, StringRef NodeName);

  /**
   * Adds an edge to the graph from the Node with id start to the Node with
   * id end.
   */
  void addEdge(unsigned FromNode, unsigned ToNode);

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

  friend std::ostream &operator<<(std::ostream &Stream, MuArchStateGraph Graph) {
  for (const auto &Nd : Graph.getNodes()) {
    Stream << Nd.second;
  }
  return Stream;
  }


  bool dump2Dot(StringRef FileName);

  /**
   * Get a list of node IDs that exist in the graph but are not mapped to any MBB.
   */
  std::vector<unsigned> getNodesNotInMBBMap() const;

  /**
   * Fill the MuArchStateGraph with nodes and edges from a MachineFunction.
   */
  bool fillMuGraphWithFunction(MachineFunction &MF, bool IsEntry,
                   const std::unordered_map<const MachineBasicBlock *, unsigned int> &MBBLatencyMap,
                   const std::unordered_map<const MachineBasicBlock *, unsigned int> &LoopBoundMap = {});

  /**
   * Fill the MuArchStateGraph with all functions from a module.
   */
  void fillMuGraph(MachineModuleInfo *MMI,
                   const std::unordered_map<const MachineBasicBlock *, unsigned int> &MBBLatencyMap,
                   const std::unordered_map<const MachineBasicBlock *, unsigned int> &LoopBoundMap = {});

  /**
   * Finalize the graph by adding call and return edges.
   */
  bool finalize(MachineFunction &MF, MachineModuleInfo *MMI);

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
   * Map that stores the entry node for each Function.
   */
  std::map<const Function *, unsigned> FunctionToEntryNodeMap;

  /**
   * Map that stores the return nodes for each Function.
   */
  std::map<const Function *, std::vector<unsigned>> FunctionToReturnNodesMap;

  /**
   * Store call sites: map from caller node to callee Function.
   */
  std::vector<std::pair<unsigned, const Function *>> CallSites;

private:
  /**
   * A counter to give unique identifiers to each Node.
   */
  unsigned NextNodeId;
};

} // end namespace llvm

#endif // MU_ARCH_STATE_GRAPH_H
