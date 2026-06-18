#ifndef LLTA_TARGETS_MSP430_MSP430FR5994TARGET_H
#define LLTA_TARGETS_MSP430_MSP430FR5994TARGET_H

#include "Targets/MSP430/MSP430Target.h"

namespace llta {

/// MSP430FR5994 device. The default device for the MSP430 family: everything
/// LLTA currently models (FRAM program memory, the FR5xx read cache, FR5994
/// cache geometry/clock defaults) targets this part. Inherits all ISA-level
/// behavior from MSP430Target; device-specific timing (FRAM memory model,
/// options) is layered on in later refactor phases.
class MSP430FR5994Target : public MSP430Target {
public:
  llvm::StringRef getName() const override;

  /// Contributes the FRAM instruction-fetch model: the wait-state pass and the
  /// FR5xx read-cache must-analysis pass. Both are no-ops unless the relevant
  /// -fram-* options are set, so default runs are unaffected.
  std::vector<llvm::MachineFunctionPass *>
  getMemoryModelPasses(llvm::TimingAnalysisResults &TAR) const override;
};

} // namespace llta

#endif // LLTA_TARGETS_MSP430_MSP430FR5994TARGET_H
