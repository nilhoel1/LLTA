#include "llvm/CodeGen/MachineFunctionPass.h"
#include <list>

namespace llvm {

class Triple;

/// Build the LLTA timing-analysis pass pipeline for the given target triple.
/// Resolves and installs the active RTTarget, then assembles the generic pass
/// skeleton plus the target's contributed memory-model passes.
std::list<MachineFunctionPass *> getTimingAnalysisPasses(const Triple &TT);

} // namespace llvm
