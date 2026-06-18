#include "Targets/TargetRegistry.h"

#include "Targets/MSP430/MSP430FR5994Target.h"
#include "Targets/RTTarget.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

namespace llta {

/// Device resolution for the MSP430 family. Only the MSP430FR5994 is
/// implemented today and it is the default for any msp430 triple. A future
/// device plugs in here (e.g. keyed on TT.getCPU() or an explicit selector)
/// without changing the arch-level keying in createRTTarget.
static std::unique_ptr<RTTarget> resolveMSP430Device(const llvm::Triple &TT) {
  (void)TT;
  return std::make_unique<MSP430FR5994Target>();
}

std::unique_ptr<RTTarget> createRTTarget(const llvm::Triple &TT) {
  switch (TT.getArch()) {
  case llvm::Triple::msp430:
    return resolveMSP430Device(TT);
  default:
    llvm::errs() << "LLTA: no timing-analysis target registered for arch '"
                 << TT.getArchName() << "'\n";
    return nullptr;
  }
}

} // namespace llta
