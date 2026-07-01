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

  /// Measured worst-case body cost for the body-less `__mspabi_*`
  /// soft-arithmetic helpers the MSP430 backend synthesizes. LLTA cannot
  /// analyze them (MSP430X-encoded, no IR body), so they are costed from
  /// hardware measurement (see ABI_FINDINGS.md). Costs the 32 helpers whose
  /// measurement is sound or trustworthy (the `ok` + `STATIC-UNSOUND` regimes)
  /// using the placement-independent `compute` column, which is valid only at
  /// -fram-wait-states=0 (NWAITS=0, RAM/<=8 MHz). Returns nullopt -- leaving
  /// the call UNSOUND -- when: -fram-wait-states >= 1 (the per-call FRAM fetch
  /// penalty is layout-dependent, with no portable upper bound), or the callee
  /// is a `no-static` FP/64-bit helper (no derivable sound ceiling) or libm.
  std::optional<unsigned>
  getExternalCallCost(llvm::StringRef CalleeName) const override;
};

} // namespace llta

#endif // LLTA_TARGETS_MSP430_MSP430FR5994TARGET_H
