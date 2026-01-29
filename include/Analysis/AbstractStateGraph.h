#ifndef ABSTRACT_STATE_GRAPH_H
#define ABSTRACT_STATE_GRAPH_H

#include "AbstractState.h"
#include "Analysis/Callbacks/CallbackManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/IR/Function.h"
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace llvm {

class AbstractStateGraph {
public:
  struct Node {
    unsigned Id;
    std::unique_ptr<AbstractState> State;
    const MachineBasicBlock *MBB;
    bool IsEntry;
    bool IsExit;
    bool IsLoopHeader;
    unsigned UpperLoopBound;
    unsigned Cost;

    Node(unsigned Id, std::unique_ptr<AbstractState> State,
         const MachineBasicBlock *MBB = nullptr)
        : Id(Id), State(std::move(State)), MBB(MBB), IsEntry(false),
          IsExit(false), IsLoopHeader(false), UpperLoopBound(0), Cost(0) {}
  };

  AbstractStateGraph() : NextNodeId(0) {}

  struct Edge {
    unsigned To;
    bool IsBackEdge;
    bool operator<(const Edge &Other) const { return To < Other.To; }
  };

  unsigned addNode(std::unique_ptr<AbstractState> State,
                   const MachineBasicBlock *MBB = nullptr);
  void addEdge(unsigned From, unsigned To, bool IsBackEdge = false);
  void removeEdge(unsigned From, unsigned To);

  Node *getNode(unsigned Id);
  const std::map<unsigned, std::unique_ptr<Node>> &getNodes() const {
    return Nodes;
  }
  const std::set<Edge> &getSuccessors(unsigned Id) const;
  const std::set<unsigned> &getPredecessors(unsigned Id) const;

  std::map<const Function *, unsigned> FunctionEntries;
  std::map<const Function *, std::vector<unsigned>> FunctionReturns;
  struct CallSite {
    unsigned CallNodeId;
    unsigned ReturnNodeId; // Successor of the call block
    const Function *Callee;
  };
  std::vector<CallSite> CallSites;

  CallbackManager &getCallbackManager() { return Callbacks; }
  const CallbackManager &getCallbackManager() const { return Callbacks; }

  void dump() const;

private:
  unsigned NextNodeId;
  std::map<unsigned, std::unique_ptr<Node>> Nodes;
  std::map<unsigned, std::set<Edge>> AdjacencyList;
  std::map<unsigned, std::set<unsigned>> Predecessors;
  CallbackManager Callbacks;
};

} // namespace llvm

#endif // ABSTRACT_STATE_GRAPH_H
