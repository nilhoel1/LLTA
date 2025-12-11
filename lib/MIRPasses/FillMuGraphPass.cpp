#include "MIRPasses/FillMuGraphPass.h"
#include "RTTargets/ProgramGraph.h"
#include "Utility/Options.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

char FillMuGraphPass::ID = 0;

FillMuGraphPass::FillMuGraphPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

Function *FillMuGraphPass::getStartingFunction(CallGraph &CG) {
  Function *StartingFunction = nullptr;
  unsigned int CurrentNumReferences = UINT_MAX;
  bool SeenNumRefsTwice = false;

  for (auto &CGNode : CG) {
    auto *F = CGNode.second->getFunction();
    if (F == nullptr)
      continue;
    if (!StartFunctionName.empty() &&
        F->getName().compare(StartFunctionName) == 0) {
      return F;
    }
    auto NumRef = CGNode.second->getNumReferences();
    if (NumRef < CurrentNumReferences) {
      StartingFunction = F;
      CurrentNumReferences = NumRef;
    } else if (NumRef == CurrentNumReferences) {
      SeenNumRefsTwice = true;
    }
  }
  if (SeenNumRefsTwice && StartFunctionName.empty())
    return nullptr;
  return StartingFunction;
}

void FillMuGraphPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<MachineModuleInfoWrapperPass>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.addRequired<MachineLoopInfoWrapperPass>();
}

bool FillMuGraphPass::runOnMachineFunction(MachineFunction &F) {
  if (StartFunctionName != "")
    FoundStartingFunction = true;
  if (!CG) {
    CG = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }
  if (!FoundStartingFunction) {
    StartingFunction = getStartingFunction(*CG);
    if (!StartingFunction) {
      outs() << "No StartingFunction found\n";
    }
  }

  auto *MMI = &getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  auto MBBLatencyMap = TAR.getMBBLatencyMap();
  auto LoopBoundMap = TAR.getLoopBoundMap();
  auto *MLI = &getAnalysis<MachineLoopInfoWrapperPass>().getLI();

  bool IsEntry = false;
  if (StartFunctionName != "") {
    if (F.getName() == StartFunctionName)
      IsEntry = true;
  } else {
    if (StartingFunction && &F.getFunction() == StartingFunction)
      IsEntry = true;
  }

  TAR.MASG.fillGraphWithFunction(F, IsEntry, MBBLatencyMap, LoopBoundMap, MLI);

  // Check if this is the last function to finalize
  bool IsLast = false;
  const Function *LastF = nullptr;
  // Find the last function that is not a declaration (has a body)
  // Because MachineFunctions are only created for functions with bodies.
  for (auto &Func : MMI->getModule()->getFunctionList()) {
    if (!Func.isDeclaration()) {
      LastF = &Func;
    }
  }

  if (LastF == &F.getFunction()) {
    IsLast = true;
  }

  if (IsLast) {
    TAR.MASG.finalize(F, MMI);
  }

  return false;
}

MachineFunctionPass *createFillMuGraphPass(TimingAnalysisResults &TAR) {
  return new FillMuGraphPass(TAR);
}

} // namespace llvm
