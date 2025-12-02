#include "llvm/CodeGen/MachineFunctionPass.h"
#include <list>

namespace llvm {

std::list<MachineFunctionPass *> getTimingAnalysisPasses();

} // namespace llvm
