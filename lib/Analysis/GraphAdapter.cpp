#include "Analysis/GraphAdapter.h"
#include "RTTargets/ProgramGraph.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Function.h"

namespace llvm {

ProgramGraph GraphAdapter::convert(AbstractStateGraph &ASG,
                                   std::map<unsigned, unsigned> &NewNodesMap) {
  ProgramGraph PG;

  // 1. Create Nodes
  for (const auto &Pair : ASG.getNodes()) {
    unsigned ASGNodeId = Pair.first;
    const auto *ASGNode = Pair.second.get();

    // Create MuArchState from Cost
    // We set UpperBound and LowerBound to the same Cost for now.
    // Ideally, Abstract Analysis could provide bounds (e.g. if we used Interval
    // Analysis).
    unsigned Cost = ASGNode->Cost;
    MuArchState State(Cost, Cost, ASGNode->State->toString());

    // Add Node to ProgramGraph
    // ProgramGraph::addNode takes (State, MBB*).
    // Note: ASGNode->MBB is strictly const MachineBasicBlock*, ProgramGraph
    // expects MachineBasicBlock*. const_cast is nasty but necessary if
    // ProgramGraph doesn't support const MBB*. Let's check
    // ProgramGraph::addNode signature. It takes MachineBasicBlock*.
    MachineBasicBlock *MBB = const_cast<MachineBasicBlock *>(ASGNode->MBB);

    unsigned PGNodeId = PG.addNode(std::make_unique<MuArchState>(State), MBB);
    NewNodesMap[ASGNodeId] = PGNodeId;

    if (MBB) {
      // Populate Function maps
      const MachineFunction *MF = MBB->getParent();
      if (MF && !MF->empty() && MBB == &MF->front()) {
        PG.FunctionToEntryNodeMap[&MF->getFunction()] = PGNodeId;
      }
      if (MBB->isReturnBlock() && MF) {
        PG.FunctionToReturnNodesMap[&MF->getFunction()].push_back(PGNodeId);
      }

      // Populate CallSites
      for (const auto &MI : *MBB) {
        if (MI.isCall()) {
          if (MI.getOperand(0).getType() == MachineOperand::MO_GlobalAddress) {
            const auto *GV = MI.getOperand(0).getGlobal();
            const auto *Callee = dyn_cast<Function>(GV);
            if (Callee) {
              PG.CallSites.push_back({PGNodeId, Callee});
            }
          }
        }
      }
    }
    // Copy name/properties if needed.
    // PG Node created by addNode doesn't set name unless we call extra method
    // or overload. But addNode handles map MBB->Node.
  }

  // 2. Add Edges
  for (const auto &Pair : ASG.getNodes()) {
    unsigned ASGNodeId = Pair.first;
    unsigned FromPGNode = NewNodesMap[ASGNodeId];

    for (const auto &Edge : ASG.getSuccessors(ASGNodeId)) {
      unsigned SuccASGId = Edge.To;
      if (NewNodesMap.count(SuccASGId)) {
        unsigned ToPGNode = NewNodesMap[SuccASGId];
        PG.addEdge(FromPGNode, ToPGNode);
      }
    }
  }

  // 3. Handle Special Nodes (Entry/Exit)?
  // ProgramGraph `fillGraphWithFunction` handles Entry/Exit explicitly.
  // AbstractStateGraph was built by Worker. Did Worker add Entry/Exit nodes?
  // Our simple Worker didn't add explicit Entry/Exit nodes unrelated to MBBs.
  // But ProgramGraph expects them for ILP solver (Entry/Exit are virtual).
  // If ASG doesn't have them, we might need to add them to PG manually or rely
  // on existing logic. The ILP solver expects a single Entry and single Exit?
  // Let's assume ASG covers the flow.
  // Ideally, we should add Entry/Exit to ASG if missing, or add them here.
  // For now, simple conversion.

  return PG;
}

} // namespace llvm
