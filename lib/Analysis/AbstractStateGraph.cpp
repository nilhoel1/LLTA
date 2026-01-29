#include "Analysis/AbstractStateGraph.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

unsigned AbstractStateGraph::addNode(std::unique_ptr<AbstractState> State,
                                     const MachineBasicBlock *MBB) {
  unsigned Id = NextNodeId++;
  Nodes[Id] = std::make_unique<Node>(Id, std::move(State), MBB);

  Callbacks.notifyNodeAdded(Id, MBB);

  return Id;
}

void AbstractStateGraph::addEdge(unsigned From, unsigned To, bool IsBackEdge) {
  if (!Callbacks.canJoinEdge(From, To)) {
    return;
  }

  AdjacencyList[From].insert({To, IsBackEdge});
  Predecessors[To].insert(From);

  Callbacks.notifyEdgeAdded(From, To, IsBackEdge);
}

void AbstractStateGraph::removeEdge(unsigned From, unsigned To) {
  // Need to find edge with To
  auto &Edges = AdjacencyList[From];
  for (auto It = Edges.begin(); It != Edges.end();) {
    if (It->To == To) {
      It = Edges.erase(It);
    } else {
      ++It;
    }
  }
  Predecessors[To].erase(From);
}

AbstractStateGraph::Node *AbstractStateGraph::getNode(unsigned Id) {
  auto It = Nodes.find(Id);
  if (It != Nodes.end()) {
    return It->second.get();
  }
  return nullptr;
}

const std::set<AbstractStateGraph::Edge> &
AbstractStateGraph::getSuccessors(unsigned Id) const {
  static const std::set<AbstractStateGraph::Edge> EmptySet;
  auto It = AdjacencyList.find(Id);
  if (It != AdjacencyList.end()) {
    return It->second;
  }
  return EmptySet;
}

const std::set<unsigned> &
AbstractStateGraph::getPredecessors(unsigned Id) const {
  static const std::set<unsigned> EmptySet;
  auto It = Predecessors.find(Id);
  if (It != Predecessors.end()) {
    return It->second;
  }
  return EmptySet;
}

void AbstractStateGraph::dump() const {
  dbgs() << "AbstractStateGraph:\n";
  for (const auto &Pair : Nodes) {
    dbgs() << "Node " << Pair.first << ": " << Pair.second->State->toString()
           << "\n";
    for (const auto &Edge : getSuccessors(Pair.first)) {
      dbgs() << "  -> " << Edge.To << (Edge.IsBackEdge ? " (BackEdge)" : "")
             << "\n";
    }
  }
}

} // namespace llvm
