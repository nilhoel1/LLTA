#ifndef LLVM_MIRPASSES_WCETANALYSISPIPELINE_H
#define LLVM_MIRPASSES_WCETANALYSISPIPELINE_H

#include "llvm/IR/Module.h"

namespace llvm {

/// Runs the "Preparation Phase" passes (canonicalization, simplification)
/// before the main WCET analysis to ensure consistent IR shape.
void runWCETAnalysisPipeline(Module &M);

} // namespace llvm

#endif // LLVM_MIRPASSES_WCETANALYSISPIPELINE_H
