#include "RTTargets/ProgramGraph.h"
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

  for (const auto &[MBB, NodeId] : MBBToNodeMap) {
    const Function *F = nullptr;

    // Check if BasicBlock is accessible and get parent function
    if (MBB) {
      const BasicBlock *BB = MBB->getBasicBlock();
      if (BB) {
        F = BB->getParent();
      }
    }

    if (F) {
      FunctionToNodes[F].push_back(NodeId);
      if (DebugPrints)
        outs() << "Mapping MBB " << MBB->getName() << " to Node ID " << NodeId
               << " in Function " << F->getName() << "\n";
    } else {
      NodesWithoutFunction.push_back(NodeId);
      if (!MBB) {
        outs() << "Mapping nullptr MBB to Node ID " << NodeId
               << " (no parent function)\n";
        assert(false && "Should not reach here!");
      }
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

bool ProgramGraph::fillGraphWithFunction(
    MachineFunction &MF, bool IsEntry,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &MBBLatencyMap,
    const std::unordered_map<const MachineBasicBlock *, unsigned int>
        &LoopBoundMap,
    MachineLoopInfo *MLI) {
  // Add entry state and Exit state
  bool EntryStateSet = false;
  bool ExitStateSet = false;
  unsigned int ExitNode;
  unsigned int EntryNode;

  if (IsEntry) {
    EntryNode = addNode(std::make_unique<MuArchState>(0, 0), nullptr);
    ExitNode = addNode(std::make_unique<MuArchState>(0, 0), nullptr);
    Nodes.at(EntryNode).setName(StringRef("Entry"));
    Nodes.at(ExitNode).setName(StringRef("Exit"));
  }

  // Fill MuArchGraph Nodes
  unsigned int CurrentNode;
  for (auto &MBB : MF) {
    // interate over MIs in MBB and find calling Instructions
    //  represented correctly by the dot file. Create a new MuArchStateGraph and
    //  add ti the graph as unique ptr
    auto It = MBBLatencyMap.find(&MBB);
    unsigned Latency = (It != MBBLatencyMap.end()) ? It->second : 0;
    addNode(std::make_unique<MuArchState>(Latency, Latency), &MBB);
    CurrentNode = MBBToNodeMap[&MBB];

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

    // Add entry state for the first Node only
    if (IsEntry && !EntryStateSet) {
      addEdge(EntryNode, CurrentNode);
      EntryStateSet = true;
    }
    // Add name for the node + Function name
    Nodes.at(CurrentNode).setName(MBB.getName());
    // Add Edges to Exit node
    if (IsEntry && MBB.isReturnBlock()) {
      addEdge(MBBToNodeMap[&MBB], ExitNode);
      ExitStateSet = true;
    }
  }
  if (IsEntry && !ExitStateSet) {
    addEdge(CurrentNode, ExitNode);
    ExitStateSet = true;
  }
  // assert(ExitStateSet && "At least one Return should have been found!");

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
            CallSites.push_back({CallNode, Callee});
          }
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

bool ProgramGraph::finalize(MachineFunction &MF, MachineModuleInfo *MMI) {
  // Process all captured call sites
  for (const auto &[CallNode, Callee] : CallSites) {
    // Use the maps to find entry and return nodes
    if (FunctionToEntryNodeMap.find(Callee) != FunctionToEntryNodeMap.end()) {
      unsigned CaleeNode = FunctionToEntryNodeMap[Callee];
      addEdge(CallNode, CaleeNode);

      if (FunctionToReturnNodesMap.find(Callee) !=
          FunctionToReturnNodesMap.end()) {
        for (unsigned ReturnNode : FunctionToReturnNodesMap[Callee]) {
          // FIXME I think this is not ideal...
          addEdge(ReturnNode, CallNode + 1);
        }
      }
    } else {
      // Fallback or error handling if callee not found in map
      // This might happen for external functions or if FillMuGraphPass didn't
      // run on it
      if (DebugPrints)
        outs() << "Warning: Callee " << Callee->getName()
               << " not found in FunctionToEntryNodeMap\n";
    }
  }
  // outs() << "Printing Dot file \n";
  dump2Dot(StringRef("ProgramGraph.dot"));
  return false;
}

} // end namespace llvm
