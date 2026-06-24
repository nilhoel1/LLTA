#include "Graph/ProgramGraph.h"
#include "Targets/RTTarget.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace llvm {

// Constructor
Node::Node(unsigned NewId, std::unique_ptr<MuArchState> State)
    : Id(NewId), State(std::move(State)) {}

// Copy Constructor
Node::Node(const Node &Node)
    : Id(Node.Id), Successors(Node.Successors), Predecessors(Node.Predecessors),
      State(std::make_unique<MuArchState>(*Node.State)) {}

// Destructor
Node::~Node() {}

// Comparison operator
bool Node::operator<(const Node &Other) const { return Id < Other.Id; }

// Get the ID of the Node
unsigned Node::getId() const { return Id; }

// Get the predecessors of the Node
const std::set<unsigned> Node::getPredecessors() const { return Predecessors; }

// Get the successors of the Node
const std::set<unsigned> Node::getSuccessors() const { return Successors; }

// Add a successor to the Node
void Node::addSuccessor(unsigned SuccessorId) {
  Successors.insert(SuccessorId);
}

// Add a predecessor to the Node
void Node::addPredecessor(unsigned PredecessorId) {
  Predecessors.insert(PredecessorId);
}

// Delete a successor from the Node
bool Node::deleteSuccessor(unsigned SuccessorId) {
  return Successors.erase(SuccessorId) > 0;
}

// Delete a predecessor from the Node
bool Node::deletePredecessor(unsigned PredecessorId) {
  return Predecessors.erase(PredecessorId) > 0;
}

// Check if a given ID is a predecessor of the Node
bool Node::isPredecessor(unsigned PredecessorId) const {
  return Predecessors.find(PredecessorId) != Predecessors.end();
}

// Check if a given ID is a successor of the Node
bool Node::isSuccessor(unsigned SuccessorId) const {
  return Successors.find(SuccessorId) != Successors.end();
}

// Check if the Node is free (no predecessors or successors)
bool Node::isFree() const { return Successors.empty() && Predecessors.empty(); }

// Get a description of the Node
std::string Node::getNodeDescr() const {
  std::string Descr = "ID: " + std::to_string(Id) + ", Name: " + Name.str() +
                      ", Cycle:" + std::to_string(State->getUpperBoundCycles());
  if (IsLoop) {
    Descr += "\\nLoop: [" + std::to_string(LowerLoopBound) + ", " +
             std::to_string(UpperLoopBound) + "]";
  }
  if (!State->DebugInfo.empty()) {
    Descr += "\\n" + State->DebugInfo;
  }
  return Descr;
}

// Get the architectural state of the Node
MuArchState &Node::getState() const { return *State; }

ProgramGraph::ProgramGraph() : Nodes(), NextNodeId(0) {}

ProgramGraph::ProgramGraph(ProgramGraph &G2)
    : Nodes(G2.Nodes), NextNodeId(G2.NextNodeId) {}

ProgramGraph::~ProgramGraph() {}

unsigned ProgramGraph::addNode(std::unique_ptr<MuArchState> State,
                               MachineBasicBlock *MBB) {
  unsigned CurrentId = NextNodeId;
  NextNodeId++;
  assert(NextNodeId > 0 &&
         "We used all Node ids for the state graph. Unsigned is not enough!");
  Node Nd(CurrentId, std::move(State));
  // Nd.setName(MBB->getName());
  Nodes.insert(std::make_pair(CurrentId, Nd));
  DEBUG_WITH_TYPE("ilp", dbgs() << "Adding Node with id " << CurrentId << "\n");
  if (MBB)
    MBBToNodeMap[MBB] = CurrentId;
  return CurrentId;
}

unsigned ProgramGraph::addNode(std::unique_ptr<MuArchState> State,
                               MachineBasicBlock *MBB, StringRef NodeName) {
  unsigned CurrentId = NextNodeId;
  NextNodeId++;
  assert(NextNodeId > 0 &&
         "We used all Node ids for the state graph. Unsigned is not enough!");
  Node Nd(CurrentId, std::move(State));
  // Nd.setName(NodeName);
  Nodes.insert(std::make_pair(CurrentId, Nd));
  DEBUG_WITH_TYPE("ilp", dbgs() << "Adding Node with id " << CurrentId << "\n");
  if (MBB)
    MBBToNodeMap[MBB] = CurrentId;
  return CurrentId;
}

void ProgramGraph::addEdge(unsigned FromNode, unsigned ToNode) {
  assert(Nodes.count(FromNode) == 1 && Nodes.count(ToNode) == 1 &&
         "Tried adding an edge between non-existent Nodes.");
  Nodes.at(FromNode).addSuccessor(ToNode);
  Nodes.at(ToNode).addPredecessor(FromNode);
}

void ProgramGraph::removeNode(unsigned Node) {
  assert(Nodes.at(Node).isFree() &&
         "Tried to remove a Node which has an edge connected!");
  Nodes.erase(Node);
}

void ProgramGraph::removeEdge(unsigned FromNode, unsigned ToNode) {
  Nodes.at(FromNode).deleteSuccessor(ToNode);
  Nodes.at(ToNode).deletePredecessor(FromNode);
}

const std::set<unsigned> ProgramGraph::getPredecessors(unsigned NodeId) const {
  return Nodes.at(NodeId).getPredecessors();
}

const std::set<unsigned> ProgramGraph::getSuccessors(unsigned NodeId) const {
  return Nodes.at(NodeId).getSuccessors();
}

const std::map<unsigned, Node> &ProgramGraph::getNodes() const { return Nodes; }

bool ProgramGraph::isFree(unsigned Node) const {
  return Nodes.at(Node).isFree();
}

bool ProgramGraph::hasEdge(unsigned FromNode, unsigned ToNode) const {
  return Nodes.at(FromNode).isSuccessor(ToNode);
}

void ProgramGraph::dump() const {
  for (const auto &Nd : Nodes) {
    errs() << Nd.second.getNodeDescr();
  }
}

bool ProgramGraph::dump2Dot(StringRef FileName) {
  std::error_code EC;
  raw_fd_ostream File(FileName, EC, sys::fs::OF_Text);
  if (EC) {
    errs() << "Error opening file: " << EC.message() << "\n";
    return false;
  }

  // Group nodes by function
  std::map<const Function *, std::vector<unsigned>> FunctionToNodes;
  std::vector<unsigned int> NodesWithoutFunction =
      getNodesNotInMBBMap(); // Nodes without a parent function

  // Use NodeToFunctionMap (populated at insertion time) instead of walking
  // MBBToNodeMap. The MBB pointers may dangle here because per-function
  // MachineFunctions are released after their pass chain runs, but the IR
  // Function pointers we captured remain valid for the module's lifetime.
  for (const auto &[NodeId, F] : NodeToFunctionMap) {
    if (F) {
      FunctionToNodes[F].push_back(NodeId);
      if (DebugPrints)
        outs() << "Mapping Node ID " << NodeId << " in Function "
               << F->getName() << "\n";
    } else {
      NodesWithoutFunction.push_back(NodeId);
    }
  }

  // Write the header
  File << "digraph MuArchStateGraph {\n";
  File << "  compound=true;\n"; // Allow edges between clusters

  unsigned ClusterId = 0;

  // Write clusters (subgraphs) for each function
  for (const auto &[MF, NodeIds] : FunctionToNodes) {
    File << "  subgraph cluster_" << ClusterId++ << " {\n";
    File << "    label=\"" << MF->getName().str() << "\";\n";
    File << "    style=filled;\n";
    File << "    color=lightgrey;\n";
    File << "    node [style=filled,color=white];\n";

    // Write nodes in this cluster
    for (unsigned NodeId : NodeIds) {
      const auto &Node = Nodes.at(NodeId);
      std::string Color = Node.IsLoop ? "lightblue" : "white";
      File << "    " << Node.getId() << " [label=\"" << Node.getNodeDescr()
           << "\",color=" << Color << "];\n";
    }

    File << "  }\n";
  }

  // Write nodes without a parent function (outside clusters)
  if (!NodesWithoutFunction.empty()) {
    File << "\n  // Nodes without parent function\n";
    File << "  node [style=filled,color=yellow];\n"; // Different style for
                                                     // orphan nodes

    for (unsigned NodeId : NodesWithoutFunction) {
      const auto &Node = Nodes.at(NodeId);
      File << "  " << Node.getId() << " [label=\"" << Node.getNodeDescr()
           << " (no function)\"];\n";
    }
  }

  // Write edges (after all clusters are defined)
  File << "\n  // Edges\n";
  for (const auto &NodePair : Nodes) {
    const auto &Node = NodePair.second;
    for (unsigned Succ : Node.getSuccessors()) {
      File << "  " << Node.getId() << " -> " << Succ << ";\n";
    }
  }

  // Write the footer
  File << "}\n";
  File.close();
  return true;
}

std::vector<unsigned> ProgramGraph::getNodesNotInMBBMap() const {
  std::vector<unsigned> NodesNotInMap;

  // Create a set of all node IDs that are in MBBToNodeMap
  std::set<unsigned> NodesInMap;
  for (const auto &[MBB, NodeId] : MBBToNodeMap) {
    NodesInMap.insert(NodeId);
  }

  // Find all nodes that are not in the MBBToNodeMap
  for (const auto &[NodeId, Node] : Nodes) {
    if (NodesInMap.find(NodeId) == NodesInMap.end()) {
      NodesNotInMap.push_back(NodeId);
    }
  }

  return NodesNotInMap;
}

bool ProgramGraph::wireEntryExit(const std::vector<unsigned> &BodyNodeIds,
                                 const std::vector<unsigned> &ReturnNodeIds,
                                 unsigned EntryNode, unsigned ExitNode) {
  bool ExitStateSet = false;

  // Entry -> first block (if the function has any blocks at all).
  if (!BodyNodeIds.empty())
    addEdge(EntryNode, BodyNodeIds.front());

  // Each return block -> Exit.
  for (unsigned Ret : ReturnNodeIds) {
    addEdge(Ret, ExitNode);
    ExitStateSet = true;
  }

  if (BodyNodeIds.empty()) {
    // Empty function (zero MachineBasicBlocks): wire Entry directly to Exit so
    // the start function still has a valid Entry->Exit path. This replaces a
    // former read of an uninitialized "CurrentNode" when the node loop never
    // ran.
    addEdge(EntryNode, ExitNode);
    ExitStateSet = true;
  } else if (!ExitStateSet) {
    // No return block (e.g. a noreturn or infinite-loop function). Fall back to
    // the last block in layout order so the graph stays connected to Exit.
    addEdge(BodyNodeIds.back(), ExitNode);
    ExitStateSet = true;
    if (Verbose)
      outs() << "  No return block found; wired last block (node "
             << BodyNodeIds.back() << ") to the Exit node as a fallback.\n";
  }

  assert(ExitStateSet && "entry function must be connected to the Exit node");
  return ExitStateSet;
}

bool ProgramGraph::fillGraphWithFunction(
    MachineFunction &MF, bool IsEntry,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &MBBLatencyMap,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &LoopBoundMap,
    MachineLoopInfo *MLI,
    const std::set<
        std::pair<const MachineBasicBlock *, const MachineBasicBlock *>>
        &IrreducibleBackEdges) {
  // Add entry state and Exit state. Only meaningful when IsEntry; the synthetic
  // Entry/Exit edges are wired by wireEntryExit() after the node loop below.
  unsigned int ExitNode = 0;
  unsigned int EntryNode = 0;

  if (IsEntry) {
    EntryNode = addNode(std::make_unique<MuArchState>(0, 0), nullptr);
    ExitNode = addNode(std::make_unique<MuArchState>(0, 0), nullptr);
    Nodes.at(EntryNode).setName(StringRef("Entry"));
    Nodes.at(ExitNode).setName(StringRef("Exit"));
    EntryNodeId = EntryNode;
    HasEntryNode = true;
    StartFunction = &MF.getFunction();
  }

  // Fill MuArchGraph Nodes. Collect the function's nodes in layout order, and
  // the subset that are return blocks; the synthetic Entry/Exit edges are then
  // wired by wireEntryExit() (a pure helper, so the empty-function and
  // no-return-block cases are unit-testable without a MachineFunction).
  std::vector<unsigned> BodyNodeIds;
  std::vector<unsigned> ReturnNodeIds;
  for (auto &MBB : MF) {
    // interate over MIs in MBB and find calling Instructions
    //  represented correctly by the dot file. Create a new MuArchStateGraph and
    //  add ti the graph as unique ptr
    auto It = MBBLatencyMap.find(&MBB);
    unsigned Latency = (It != MBBLatencyMap.end()) ? It->second : 0;
    addNode(std::make_unique<MuArchState>(Latency, Latency), &MBB);
    unsigned CurrentNode = MBBToNodeMap[&MBB];
    NodeToFunctionMap[CurrentNode] = &MF.getFunction();
    BodyNodeIds.push_back(CurrentNode);

    // Check if this MBB is a loop header and set loop bounds
    auto LoopIt = LoopBoundMap.find(&MBB);
    if (LoopIt != LoopBoundMap.end()) {
      Nodes.at(CurrentNode).IsLoop = true;
      Nodes.at(CurrentNode).UpperLoopBound = LoopIt->second;
      Nodes.at(CurrentNode).LowerLoopBound = 1; // Default lower bound
      if (DebugPrints) {
        outs() << "  Marked node " << CurrentNode << " (MBB " << MBB.getName()
               << ") as loop header with bound " << LoopIt->second << "\n";
      }
    }

    // Add name for the node + Function name
    Nodes.at(CurrentNode).setName(MBB.getName());
    if (MBB.isReturnBlock())
      ReturnNodeIds.push_back(CurrentNode);
  }
  if (IsEntry)
    wireEntryExit(BodyNodeIds, ReturnNodeIds, EntryNode, ExitNode);

  // Fill MuArchGraph Edges
  for (auto &MBB : MF) {
    for (auto &Succ : MBB.successors()) {
      // if (Succ->getParent() == MBB.getParent()){
      unsigned FromNode = MBBToNodeMap[&MBB];
      unsigned ToNode = MBBToNodeMap[Succ];
      addEdge(FromNode, ToNode);

      // Check for backedge using MachineLoopInfo
      if (MLI) {
        MachineLoop *L = MLI->getLoopFor(Succ);
        if (L && L->getHeader() == Succ && L->contains(&MBB)) {
          // This is a backedge from MBB to Succ (Header)
          Nodes.at(ToNode).BackEdgePredecessors.insert(FromNode);
          if (DebugPrints) {
            outs() << "  Identified backedge: " << FromNode << " -> " << ToNode
                   << "\n";
          }
        }
      }

      // Backedge of an irreducible loop MachineLoopInfo did not recognize (e.g.
      // a switch jump-table forming a multi-entry loop -- Duff's device). The
      // loop-bound pass found these via a DFS retreating-edge scan; flag the
      // loop-closing predecessor here so the IPET loop-bound row is emitted for
      // the (LoopBoundMap-bounded) header.
      if (IrreducibleBackEdges.count({&MBB, Succ})) {
        Nodes.at(ToNode).BackEdgePredecessors.insert(FromNode);
        if (DebugPrints)
          outs() << "  Identified irreducible backedge: " << FromNode << " -> "
                 << ToNode << "\n";
      }
      //}
    }
  }

  // Store Entry and Return nodes for this function
  if (!MF.empty()) {
    FunctionToEntryNodeMap[&MF.getFunction()] = MBBToNodeMap[&*MF.begin()];
  }
  for (auto &MBB : MF) {
    if (MBB.isReturnBlock()) {
      FunctionToReturnNodesMap[&MF.getFunction()].push_back(MBBToNodeMap[&MBB]);
    }
    // Capture call sites
    for (auto &MI : MBB) {
      if (MI.isCall()) {
        if (MI.getOperand(0).getType() ==
            llvm::MachineOperand::MO_GlobalAddress) {
          const auto *GV = MI.getOperand(0).getGlobal();
          const auto *Callee = dyn_cast<Function>(GV);
          if (Callee) {
            unsigned CallNode = MBBToNodeMap[&MBB];
            // The return lands in the call block's continuation successor (the
            // block CallSplitterPass placed right after the call). Capture its
            // node now rather than guessing CallNode+1 in finalize(): node IDs
            // follow MBB-layout order, so CallNode+1 is only the landing block
            // by accident and points into the next function when the call is
            // the last block in this function's layout.
            const MachineBasicBlock *Landing =
                MBB.succ_size() == 1 ? *MBB.succ_begin() : MBB.getFallThrough();
            unsigned LandingNode = 0;
            bool HasLanding = false;
            if (Landing && MBBToNodeMap.count(Landing)) {
              LandingNode = MBBToNodeMap[Landing];
              HasLanding = true;
            }
            CallSites.push_back({CallNode, Callee, LandingNode, HasLanding});
          }
        } else if (MI.getOperand(0).getType() ==
                   llvm::MachineOperand::MO_ExternalSymbol) {
          // Backend-synthesized libcall (e.g. __mspabi_* soft-float). No IR
          // Function exists, so it is captured by symbol name and resolved in
          // finalize() against the target's external-call cost model.
          unsigned CallNode = MBBToNodeMap[&MBB];
          ExternalSymbolCallSites.push_back(
              {CallNode, std::string(MI.getOperand(0).getSymbolName())});
        }
      }
    }
  }

  return true;
}

void ProgramGraph::fillGraph(
    MachineModuleInfo *MMI,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &MBBLatencyMap,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &LoopBoundMap) {
  // Fill the Mu graph from MBBs
  bool IsEntry = true;
  for (auto &F : MMI->getModule()->getFunctionList()) {
    if (auto *MF = MMI->getMachineFunction(F)) {
      fillGraphWithFunction(*MF, IsEntry, MBBLatencyMap, LoopBoundMap);
      if (IsEntry)
        IsEntry = false;
    }
  }
}

bool ProgramGraph::finalize(
    MachineFunction &MF, MachineModuleInfo *MMI, const llta::RTTarget *Target,
    const std::map<std::string, unsigned> &RecursionBounds) {
  // Body-less external calls (declared-but-undefined IR functions like libm
  // sin/cos, and backend MO_ExternalSymbol libcalls like __mspabi_*) have no
  // node in the graph. Charge the target's cost for them if it has one, else
  // stage the name: reachable un-costed callees make the WCET an
  // under-approximation and are reported as unsound. Staging is resolved after
  // pruning so only callees on a reachable path count.
  std::vector<std::pair<unsigned, std::string>> PendingUnsound;
  auto chargeOrStage = [&](unsigned CallNode, StringRef Name) {
    if (Target) {
      if (std::optional<unsigned> Cost = Target->getExternalCallCost(Name)) {
        // The callee body cost is added to the call-site node's upper (and
        // lower) bound; the `call` instruction itself is already costed.
        MuArchState &St = Nodes.at(CallNode).getState();
        St.MaxCycles += *Cost;
        St.MinCycles += *Cost;
        return;
      }
    }
    PendingUnsound.push_back({CallNode, Name.str()});
  };

  // Recursion findings, collected by IR Function during wiring and filtered to
  // the reachable subgraph after pruning (so dead-code recursion does not
  // produce a spurious "unbounded" diagnostic when the live WCET is finite).
  std::set<const Function *> BoundedSelfRec, UnboundedSelfRec, MutualRec;

  // Process all captured call sites
  for (const auto &[CallNode, Callee, LandingNode, HasLanding] : CallSites) {
    // Never wire calls *into* the start function. It is the analysis boundary:
    // it is entered only via the synthetic Entry node and left only via the
    // synthetic Exit node. Wiring such a call site (an external caller of the
    // start function) would add a return edge from the start function back into
    // its caller, dragging the caller's (possibly unbounded) loops into the
    // reachable subgraph.
    if (Callee == StartFunction)
      continue;
    // Use the maps to find entry and return nodes
    if (FunctionToEntryNodeMap.find(Callee) != FunctionToEntryNodeMap.end()) {
      unsigned CaleeNode = FunctionToEntryNodeMap[Callee];
      addEdge(CallNode, CaleeNode);

      // Wire each of the callee's return blocks back to this call site's
      // return-landing block (captured at call detection). A call with no
      // continuation successor (HasLanding == false, e.g. a noreturn call) gets
      // no return edge.
      if (HasLanding && FunctionToReturnNodesMap.find(Callee) !=
                            FunctionToReturnNodesMap.end()) {
        for (unsigned ReturnNode : FunctionToReturnNodesMap[Callee])
          addEdge(ReturnNode, LandingNode);
      }

      // Self-recursion: the call edge closes a cycle back into the callee's own
      // entry. Treat that entry like a bounded loop header whose backedge is
      // the recursive call edge, so the existing IPET loop-bound row caps total
      // invocations. The bound (max total invocations per top-level call) comes
      // from `#pragma recursion_bound(N)`. Without a bound the cycle is
      // unbounded -- record it for diagnosis rather than emitting an unsolvable
      // (unbounded) ILP silently.
      auto It = NodeToFunctionMap.find(CallNode);
      const Function *Caller =
          It != NodeToFunctionMap.end() ? It->second : nullptr;
      if (Caller && Caller == Callee) {
        auto RB = RecursionBounds.find(Callee->getName().str());
        if (RB != RecursionBounds.end() && RB->second > 0) {
          Node &Entry = Nodes.at(CaleeNode);
          Entry.IsLoop = true;
          Entry.UpperLoopBound = RB->second;
          Entry.BackEdgePredecessors.insert(CallNode);
          BoundedSelfRec.insert(Callee);
        } else {
          UnboundedSelfRec.insert(Callee);
        }
      }
    } else if (Callee->isDeclaration()) {
      // Declared-but-undefined function whose body is not in the analyzed IR
      // (e.g. libm). Cost it via the target model or flag it as unsound.
      chargeOrStage(CallNode, Callee->getName());
    } else if (DebugPrints) {
      // A defined function with no entry node would be a different bug
      // (FillMuGraphPass did not run on it); leave it as a diagnostic.
      outs() << "Warning: Callee " << Callee->getName()
             << " not found in FunctionToEntryNodeMap\n";
    }
  }

  // Detect mutual recursion (multi-function call-graph cycles). Build the
  // function-level call adjacency from the captured call sites, then flag any
  // function F that calls some G != F where G can reach back to F. Such cycles
  // are not bounded in this phase (only direct self-recursion is) -- report
  // them so the unbounded ILP failure is diagnosable.
  std::map<const Function *, std::set<const Function *>> CallAdj;
  for (const auto &CS : CallSites) {
    auto It = NodeToFunctionMap.find(CS.CallNode);
    const Function *Caller =
        It != NodeToFunctionMap.end() ? It->second : nullptr;
    if (Caller && CS.Callee)
      CallAdj[Caller].insert(CS.Callee);
  }
  auto reaches = [&](const Function *Src, const Function *Dst) {
    std::set<const Function *> Visited;
    std::vector<const Function *> WL{Src};
    while (!WL.empty()) {
      const Function *Cur = WL.back();
      WL.pop_back();
      if (Cur == Dst)
        return true;
      auto AdjIt = CallAdj.find(Cur);
      if (AdjIt == CallAdj.end())
        continue;
      for (const Function *N : AdjIt->second)
        if (Visited.insert(N).second)
          WL.push_back(N);
    }
    return false;
  };
  for (const auto &[Caller, Callees] : CallAdj)
    for (const Function *Callee : Callees)
      if (Caller != Callee && reaches(Callee, Caller)) {
        MutualRec.insert(Caller);
        MutualRec.insert(Callee);
      }

  // Backend-synthesized libcalls (no IR Function): cost by symbol name.
  for (const auto &[CallNode, Name] : ExternalSymbolCallSites)
    chargeOrStage(CallNode, Name);
  // Prune nodes that are not reachable from the Entry node. Because every
  // MachineFunction in the module is accumulated into this single graph, the
  // functions that are not part of the start function's call subgraph (and
  // their possibly unbounded loops) would otherwise remain as disconnected
  // components and make the WCET ILP unbounded.
  if (HasEntryNode) {
    // Forward BFS from the Entry node over successor edges.
    std::set<unsigned> Reachable;
    std::vector<unsigned> Worklist;
    Reachable.insert(EntryNodeId);
    Worklist.push_back(EntryNodeId);
    while (!Worklist.empty()) {
      unsigned Cur = Worklist.back();
      Worklist.pop_back();
      // getSuccessors() returns a copy, so it is safe to traverse here.
      for (unsigned Succ : Nodes.at(Cur).getSuccessors()) {
        if (Reachable.insert(Succ).second)
          Worklist.push_back(Succ);
      }
    }

    // Collect the unreachable nodes first to avoid mutating Nodes while
    // iterating over it.
    std::vector<unsigned> ToRemove;
    for (const auto &[Id, Nd] : Nodes) {
      if (Reachable.find(Id) == Reachable.end())
        ToRemove.push_back(Id);
    }

    for (unsigned Id : ToRemove) {
      // Detach all edges so removeNode()'s isFree() assertion holds. Iterate
      // over copies (getSuccessors/getPredecessors return by value) because
      // removeEdge mutates the underlying sets. A reachable node can never
      // point to an unreachable one, so only this node's own edges exist.
      for (unsigned Succ : Nodes.at(Id).getSuccessors())
        removeEdge(Id, Succ);
      for (unsigned Pred : Nodes.at(Id).getPredecessors())
        removeEdge(Pred, Id);
      NodeToFunctionMap.erase(Id);
      removeNode(Id);
    }

    if (Verbose)
      outs() << "Pruned " << ToRemove.size()
             << " node(s) unreachable from the start function; " << Nodes.size()
             << " node(s) remain.\n";
  }

  // Keep only un-costed external calls that survived pruning (i.e. lie on a
  // reachable path); those are the ones that make the reported WCET unsound.
  for (const auto &[CallNode, Name] : PendingUnsound)
    if (Nodes.find(CallNode) != Nodes.end())
      UnsoundExternalCallees.insert(Name);

  // Surface recursion findings, but only for functions that survived pruning
  // (i.e. lie on a reachable analysis path). A function survives iff its entry
  // node is still present. Bounded self-recursion is informational; the
  // unbounded cases (self-recursion without a `recursion_bound`, and any mutual
  // recursion) make the WCET non-existent, so name them clearly.
  auto Reachable = [&](const Function *F) {
    auto It = FunctionToEntryNodeMap.find(F);
    return It != FunctionToEntryNodeMap.end() && Nodes.count(It->second);
  };
  for (const Function *F : BoundedSelfRec)
    if (Reachable(F)) {
      BoundedRecursionFunctions.insert(F->getName().str());
      outs() << "RECURSION: bounded self-recursive function '" << F->getName()
             << "' via recursion_bound.\n";
    }
  for (const Function *F : UnboundedSelfRec)
    if (Reachable(F)) {
      UnboundedRecursionFunctions.insert(F->getName().str());
      outs()
          << "RECURSION: function '" << F->getName()
          << "' is self-recursive but has no recursion_bound; its call cycle "
             "is UNBOUNDED (no WCET). Add #pragma recursion_bound(N).\n";
    }
  std::set<std::string> ReachableMutual;
  for (const Function *F : MutualRec)
    if (Reachable(F))
      ReachableMutual.insert(F->getName().str());
  if (!ReachableMutual.empty()) {
    MutualRecursionFunctions = ReachableMutual;
    outs() << "RECURSION: mutual recursion detected among {";
    bool First = true;
    for (const auto &Name : ReachableMutual) {
      outs() << (First ? "" : ", ") << Name;
      First = false;
    }
    outs() << "}; multi-function cycles are not bounded in this phase, so the "
              "WCET is UNBOUNDED.\n";
  }

  // outs() << "Printing Dot file \n";
  dump2Dot(StringRef("ProgramGraph.dot"));
  return false;
}

} // end namespace llvm
