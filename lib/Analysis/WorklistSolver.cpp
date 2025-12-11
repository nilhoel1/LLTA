#include "Analysis/WorklistSolver.h"
#include "RTTargets/ProgramGraph.h" // For access to ProgramGraph utilities if needed, but we used AbstractStateGraph
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include <map>

namespace llvm {

void WorklistSolver::addToWorklist(unsigned NodeId) {
  if (InWorklist.find(NodeId) == InWorklist.end()) {
    Worklist.push_back(NodeId);
    InWorklist.insert(NodeId);
  }
}

unsigned WorklistSolver::takeFromWorklist() {
  unsigned NodeId = Worklist.front();
  Worklist.pop_front();
  InWorklist.erase(NodeId);
  return NodeId;
}

void WorklistSolver::initializeGraph(
    MachineFunction &MF, MachineLoopInfo *MLI,
    const std::map<const MachineBasicBlock *, unsigned> *LoopBounds) {
  // Clear existing graph? Or expected to start fresh?
  // For now, assume a fresh run or we append.

  // Create nodes
  // This is a simplified initialization. In a real integration, we might
  // need to mirror the CFG structure more closely or use the ProgramGraph
  // structure. BUT the AbstractStateGraph is built AS WE GO or pre-built? Plan
  // says: Worker executes the analysis. Usually, we map MBBs to
  // AbstractStateGraph Nodes.

  // Let's assume we build the graph structure based on the CFG first
  // OR we just use the CFG to drive the analysis and build the ASG nodes.

  // Implementation choice: Build ASG structure mirroring CFG first.
  std::map<const MachineBasicBlock *, unsigned> MBBToNodeMap;

  for (auto &MBB : MF) {
    auto InitialState = Analysis.getInitialState();
    unsigned NodeId = Graph.addNode(std::move(InitialState), &MBB);
    MBBToNodeMap[&MBB] = NodeId;

    // Set Loop Info
    if (MLI && LoopBounds) {
      MachineLoop *L = MLI->getLoopFor(&MBB);
      if (L && L->getHeader() == &MBB) {
        if (auto *N = Graph.getNode(NodeId)) {
          N->IsLoopHeader = true;
          // Get bound from TAR
          if (LoopBounds->count(&MBB)) {
            N->UpperLoopBound = LoopBounds->at(&MBB);
          }
        }
      }
    }
  }

  // Add edges
  for (auto &MBB : MF) {
    unsigned From = MBBToNodeMap[&MBB];
    for (auto *Succ : MBB.successors()) {
      unsigned To = MBBToNodeMap[Succ];

      bool IsBack = false;
      if (MLI) {
        // A simplistic backedge check: if flow goes to the header of the loop
        // containing the source block (and it's the same loop or outer).
        MachineLoop *L = MLI->getLoopFor(&MBB);
        if (L && L->getHeader() == Succ) {
          IsBack = true;
        }
      }

      Graph.addEdge(From, To, IsBack);
    }
  }

  // Initialize worklist with entry block
  if (!MF.empty()) {
    unsigned EntryId = MBBToNodeMap[&MF.front()];
    addToWorklist(EntryId);
    if (auto *N = Graph.getNode(EntryId)) {
      N->IsEntry = true;
    }
    Graph.FunctionEntries[&MF.getFunction()] = EntryId;
  }

  // Set IsExit and populated IP maps
  for (auto &MBB : MF) {
    unsigned NodeId = MBBToNodeMap[&MBB];
    if (MBB.isReturnBlock()) {
      if (auto *N = Graph.getNode(NodeId)) {
        N->IsExit = true;
      }
      Graph.FunctionReturns[&MF.getFunction()].push_back(NodeId);
    }

    // Check for Calls
    for (const auto &MI : MBB) {
      if (MI.isCall()) {
        // Check for GlobalAddress operand
        for (const auto &Op : MI.operands()) {
          if (Op.isGlobal() && isa<Function>(Op.getGlobal())) {
            const Function *Callee = cast<Function>(Op.getGlobal());
            // Assume Successors[0] is the return point?
            // If CallSplitter is used, basic block ends after call.
            // Successors are the return block(s).
            for (auto *Succ : MBB.successors()) {
              Graph.CallSites.push_back({NodeId, MBBToNodeMap[Succ], Callee});
            }
          }
        }
      }
    }
  }
}

void WorklistSolver::initializeGraph(const ProgramGraph &PG) {
  // Map PG Node ID -> ASG Node ID
  std::map<unsigned, unsigned> PGToASGMap;

  // Build ID -> MBB map for PG
  std::map<unsigned, const MachineBasicBlock *> NodeToMBBMap;
  for (const auto &Pair : PG.MBBToNodeMap) {
    NodeToMBBMap[Pair.second] = Pair.first;
  }

  // 1. Create Nodes
  for (const auto &Pair : PG.getNodes()) {
    const auto &PGNode = Pair.second;

    auto InitialState = Analysis.getInitialState();
    // Use const_cast because AbstractStateGraph currently stores non-const
    // MBB*? Check definition: Node stores MachineBasicBlock *MBB.

    // CRITICAL FIX: The MBB pointers in ProgramGraph might be dangling (if
    // functions are deleted). The ProgramGraph nodes already contain the
    // computed Latency/Cost in their State. We should use that Cost and NOT
    // attempt to re-process instructions (which would segfault). We pass
    // nullptr for MBB to signal "No instructions available / Summarized Node".
    unsigned ASGNodeId =
        Graph.addNode(std::move(InitialState),
                      nullptr /* const_cast<MachineBasicBlock *>(MBB) */);
    PGToASGMap[PGNode.Id] = ASGNodeId;

    if (auto *N = Graph.getNode(ASGNodeId)) {
      // N->Name does not exist.
      // Copy Cost from ProgramGraph Node State
      N->Cost = PGNode.getState().getUpperBoundCycles();

      if (!PGNode.BackEdgePredecessors.empty()) {
        N->IsLoopHeader = true;
        N->UpperLoopBound = PGNode.UpperLoopBound;
      }
      if (PGNode.Name == "Entry")
        N->IsEntry = true;
      if (PGNode.Name == "Exit")
        N->IsExit = true;
    }
  }

  // 2. Add Edges
  for (const auto &Pair : PG.getNodes()) {
    const auto &PGNode = Pair.second;
    unsigned FromASG = PGToASGMap[PGNode.Id];

    for (unsigned SuccId : PGNode.getSuccessors()) {
      if (PGToASGMap.count(SuccId)) {
        unsigned ToASG = PGToASGMap[SuccId];

        bool IsBackEdge = false;
        // Check if Succ considers THIS node a BackEdgePredecessor
        if (PG.getNodes().count(SuccId)) {
          const auto &SuccNode = PG.getNodes().at(SuccId);
          if (SuccNode.BackEdgePredecessors.count(PGNode.Id)) {
            IsBackEdge = true;
          }
        }
        Graph.addEdge(FromASG, ToASG, IsBackEdge);
      }
    }
  }

  // 3. Set Entry/Exit
  for (const auto &Pair : PG.getNodes()) {
    const auto &PGNode = Pair.second;
    unsigned ASGNodeId = PGToASGMap[PGNode.Id];
    if (auto *N = Graph.getNode(ASGNodeId)) {
      if (PGNode.getPredecessors().empty())
        N->IsEntry = true;
      if (PGNode.getSuccessors().empty())
        N->IsExit = true;
    }
  }

  // 4. IP Info
  for (const auto &Pair : PG.FunctionToEntryNodeMap) {
    if (PGToASGMap.count(Pair.second)) {
      Graph.FunctionEntries[Pair.first] = PGToASGMap[Pair.second];
    }
  }
  for (const auto &Pair : PG.FunctionToReturnNodesMap) {
    for (unsigned RetID : Pair.second) {
      if (PGToASGMap.count(RetID)) {
        Graph.FunctionReturns[Pair.first].push_back(PGToASGMap[RetID]);
      }
    }
  }
  for (const auto &CS : PG.CallSites) {
    (void)CS; // Suppress unused variable warning
    // Note: Call site edges are already in PG (finalized).
    // If we need to populate ASG.CallSites for inter-procedural splicing,
    // we would map CS.first to an ASG node ID here.
  }

  // Worklist: Add all entries
  for (const auto &Pair : Graph.getNodes()) {
    if (Pair.second->IsEntry) {
      addToWorklist(Pair.first);
    }
  }

  // Identifiy Entry/Exit for ASG (if needed) from PG names
  // ...

  llvm::errs() << "Initialized ASG from PG: " << Graph.getNodes().size()
               << " nodes.\n";
}

void WorklistSolver::run(const ProgramGraph &PG) {
  initializeGraph(PG);

  // Initialize worklist with Entry node (assumed 0 or find it)
  // PG Entry is usually 0? Check PG.
  // We need to find the node corresponding to PG Entry.
  // In initializeGraph we iterated PG.getNodes(). The mapping is PGToASGMap.
  // We should find the entry node.

  // For now, assume simple iteration adds them.
  // Graph.getNodes() is a map.

  if (Graph.getNodes().empty())
    return;

  // Add all nodes to worklist initially? Or just entry?
  // Standard worklist: Add Entry.
  // Since we rely on PGToASGMap which is local to initializeGraph,
  // we might need to expose it or iterate keys.

  // Let's add ALL nodes to worklist to be safe, or find Entry.
  for (auto &Pair : Graph.getNodes()) {
    addToWorklist(Pair.first);
  }

  llvm::errs() << "Starting Worklist Algorithm...\n";

  while (!Worklist.empty()) {
    unsigned NodeId = takeFromWorklist();
    auto *Node = Graph.getNode(NodeId);
    if (!Node)
      continue;

    // 1. Join predecessors
    std::unique_ptr<AbstractState> InState = Analysis.getInitialState();
    bool FirstPred = true;
    for (unsigned PredId : Graph.getPredecessors(NodeId)) {
      auto *PredNode = Graph.getNode(PredId);
      if (PredNode) {
        if (FirstPred) {
          InState = PredNode->State->clone();
          FirstPred = false;
        } else {
          InState->join(PredNode->State.get());
        }
      }
    }

    // 2. Process
    unsigned BlockCost = 0;
    if (Node->MBB) {
      // llvm::errs() << "Processing Node " << NodeId << " with MBB " <<
      // Node->MBB->getName() << "\n";
      for (const auto &MI : *Node->MBB) {
        unsigned InstCost = Analysis.process(InState.get(), &MI);
        BlockCost += InstCost;
      }
      // llvm::errs() << "  Cost: " << BlockCost << "\n";
      Node->Cost = BlockCost;
    } else {
      // llvm::errs() << "Processing Node " << NodeId << " (No MBB)\n";
      // If MBB is null, we assume the Cost is already set (e.g. from
      // ProgramGraph) and we just propagate the state (Identity). Keep existing
      // Node->Cost.
    }

    // 3. Update
    if (!Node->State->equals(InState.get())) {
      Node->State = std::move(InState);
      for (const auto &Edge : Graph.getSuccessors(NodeId)) {
        addToWorklist(Edge.To);
      }
    }
  }

  llvm::errs() << "Worklist Analysis Complete.\n";
}

void WorklistSolver::run(
    MachineFunction &MF, MachineLoopInfo *MLI,
    const std::map<const MachineBasicBlock *, unsigned> *LoopBounds) {
  initializeGraph(MF, MLI, LoopBounds);

  while (!Worklist.empty()) {
    unsigned NodeId = takeFromWorklist();
    auto *Node = Graph.getNode(NodeId);
    if (!Node)
      continue;

    // 1. Join predecessors (meet operator)
    std::unique_ptr<AbstractState> InState =
        Analysis.getInitialState(); // Default bottom/top?

    // Actually we should start with a clean state or clone from one predecessor
    // and join others.
    bool FirstPred = true;
    for (unsigned PredId : Graph.getPredecessors(NodeId)) {
      auto *PredNode = Graph.getNode(PredId);
      if (PredNode) {
        if (FirstPred) {
          InState = PredNode->State->clone();
          FirstPred = false;
        } else {
          InState->join(PredNode->State.get());
        }
      }
    }

    if (FirstPred) {
      // No predecessors, keep initial state (if entry) or bottom
      // If entry, it acts as initial state.
      // If we strictly follow the init above, the node already has a state.
      // But that state is "Initial".
    }

    // 2. Process the block (Transfer Function)
    // We modify InState in place
    unsigned BlockCost = 0;
    if (Node->MBB) {
      for (const auto &MI : *Node->MBB) {
        BlockCost += Analysis.process(InState.get(), &MI);
      }
    }

    // Check if Cost changed? Cost is not part of state equality check usually,
    // but part of properties. We update it always.
    Node->Cost = BlockCost;

    // 3. Check for change and update
    if (!Node->State->equals(InState.get())) {
      Node->State = std::move(InState);
      // Add successors to worklist
      for (const auto &Edge : Graph.getSuccessors(NodeId)) {
        addToWorklist(Edge.To);
      }
    }
  }
}

} // namespace llvm
