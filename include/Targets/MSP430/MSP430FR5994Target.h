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

  /// Measured worst-case body cost for the body-less integer `__mspabi_*`
  /// soft-arithmetic helpers (mul/div/mod/shift) the MSP430 backend
  /// synthesizes. LLTA cannot analyze them (MSP430X-encoded, no IR body), so
  /// they are costed from hardware measurement (see ABI_FINDINGS.md). The
  /// deployment column is chosen by -fram-wait-states: 0 -> compute (NWAITS=0,
  /// RAM/<=8 MHz), 1 -> fram_tot (NWAITS=1, FRAM @16 MHz). Returns nullopt for
  /// any other wait-state value (unmeasured) and for float/double/libm callees,
  /// leaving them UNSOUND.
  std::optional<unsigned>
  getExternalCallCost(llvm::StringRef CalleeName) const override;
};

} // namespace llta

#endif // LLTA_TARGETS_MSP430_MSP430FR5994TARGET_H
