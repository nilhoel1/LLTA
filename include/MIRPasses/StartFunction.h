#ifndef LLTA_MIRPASSES_STARTFUNCTION_H
#define LLTA_MIRPASSES_STARTFUNCTION_H

#include "llvm/Analysis/CallGraph.h"

namespace llvm {

/// Determine the entry/start function for the timing analysis.
///
/// Precedence:
///   1. the explicit `-start-function` override, if set and present;
///   2. a function literally named `main`;
///   3. the function with the fewest CallGraph references (the entry point is
///      typically uncalled within the module), breaking ties deterministically
///      by name.
///
/// Returns nullptr only when the call graph contains no defined function.
/// Note: for a self-contained module `main` is uncalled, so it is already the
/// minimum-reference node -- (2) only changes behaviour on a reference-count
/// tie, which previously yielded no start function at all.
Function *findStartFunction(CallGraph &CG);

} // namespace llvm

#endif // LLTA_MIRPASSES_STARTFUNCTION_H
