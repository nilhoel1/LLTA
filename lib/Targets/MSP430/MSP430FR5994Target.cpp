#include "Targets/MSP430/MSP430FR5994Target.h"

#include "Targets/MSP430/FRAMCacheAnalysisPass.h"
#include "Targets/MSP430/FRAMWaitStatePass.h"

namespace llta {

llvm::StringRef MSP430FR5994Target::getName() const { return "MSP430FR5994"; }

std::vector<llvm::MachineFunctionPass *>
MSP430FR5994Target::getMemoryModelPasses(llvm::TimingAnalysisResults &TAR) const {
  // Order matters: the wait-state pass runs first; the cache analysis, when
  // enabled, supersedes it (the wait-state pass then skips itself).
  return {llvm::createFRAMWaitStatePass(TAR),
          llvm::createFRAMCacheAnalysisPass(TAR)};
}

} // namespace llta
