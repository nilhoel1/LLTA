#include "MIRPasses/StartFunction.h"
#include "Utility/Options.h"
#include "llvm/IR/Function.h"
#include <climits>

namespace llvm {

Function *findStartFunction(CallGraph &CG) {
  Function *Override = nullptr; // the -start-function override, if present
  Function *Main = nullptr;     // a function literally named "main"
  Function *MinRef = nullptr;   // fewest references (tie: smaller name)
  unsigned MinNumRef = UINT_MAX;

  for (auto &Entry : CG) {
    Function *F = Entry.second->getFunction();
    if (F == nullptr)
      continue;

    if (!StartFunctionName.empty() && F->getName() == StartFunctionName)
      Override = F;
    if (F->getName() == "main")
      Main = F;

    unsigned NumRef = Entry.second->getNumReferences();
    if (MinRef == nullptr || NumRef < MinNumRef ||
        (NumRef == MinNumRef && F->getName() < MinRef->getName())) {
      MinRef = F;
      MinNumRef = NumRef;
    }
  }

  if (Override != nullptr)
    return Override;
  if (Main != nullptr)
    return Main;
  return MinRef;
}

} // namespace llvm
