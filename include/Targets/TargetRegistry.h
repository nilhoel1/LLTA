#ifndef LLTA_TARGETS_TARGETREGISTRY_H
#define LLTA_TARGETS_TARGETREGISTRY_H

#include <memory>

namespace llvm {
class Triple;
} // namespace llvm

namespace llta {

class RTTarget;

/// Resolve the concrete RTTarget for an LLVM target triple. Two stages:
///   1. the arch (Triple::getArch) selects a target *family*;
///   2. a per-family device-resolution step selects the concrete *device*,
///      defaulting to the device LLTA currently implements for that family.
///
/// This is the single extension point for new targets: register a family for
/// an arch and (optionally) refine its device resolution. Returns nullptr and
/// diagnoses on errs() for an arch with no registered family.
std::unique_ptr<RTTarget> createRTTarget(const llvm::Triple &TT);

} // namespace llta

#endif // LLTA_TARGETS_TARGETREGISTRY_H
