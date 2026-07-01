#include "Targets/MSP430/MSP430FR5994Target.h"

#include "Targets/MSP430/FRAMCacheAnalysisPass.h"
#include "Targets/MSP430/FRAMWaitStatePass.h"
#include "Targets/MSP430/MSP430Options.h"

#include "llvm/ADT/StringMap.h"

namespace llta {

llvm::StringRef MSP430FR5994Target::getName() const { return "MSP430FR5994"; }

std::vector<llvm::MachineFunctionPass *>
MSP430FR5994Target::getMemoryModelPasses(
    llvm::TimingAnalysisResults &TAR) const {
  // Order matters: the wait-state pass runs first; the cache analysis, when
  // enabled, supersedes it (the wait-state pass then skips itself).
  return {llvm::createFRAMWaitStatePass(TAR),
          llvm::createFRAMCacheAnalysisPass(TAR)};
}

// Measured worst-case callee *body* cost for the body-less __mspabi_*
// soft-arithmetic helpers, from lib/Targets/MSP430/ABI_FINDINGS.md
// (MSP-EXP430FR5994 Launchpad, Timer_B0 single-call method at that routine's
// worst-case input; the `call` instruction itself is costed separately at the
// call site). Values are the `compute` column: 8 MHz, NWAITS=0 -- the bound for
// a RAM-resident or <=8 MHz deployment (-fram-wait-states=0). `compute` is
// cache- and placement-independent (re-measurement reproduced every value
// bit-identically); the `fram_tot` column is deliberately not used -- it is
// layout-dependent and NOT a sound upper bound (a different link order can
// exceed it; see ABI_FINDINGS.md, "fram_tot is layout-dependent").
//
// Only the 32 helpers in ABI_FINDINGS.md's `ok` and `STATIC-UNSOUND` regimes
// are listed: the `ok` set has a sound static ceiling and measured <= it, and
// the `STATIC-UNSOUND` shifts have a trustworthy measurement (scripts/
// derive_abi_costs.py just fails to see their loop). The 22 `no-static` helpers
// -- 64-bit mul/div (mpyll/divlli/divull/remull), all float/double arithmetic
// (add/sub/mpy/div/cmp, single and double), and the 64-bit int<->float
// conversions (fltllif/fltullf/fltllid/fltulld, fixflli/fixdlli/fixdul/fixdull)
// -- are deliberately absent: their soft-float tree reaches a statically
// unresolvable indirect call, so no sound ceiling is derivable and the bound is
// empirical only (see docs/LibraryCallCosting.md). They, and libm, stay
// UNSOUND. Note the per-routine asymmetry this produces: 64-bit `remlli` is
// costed while its siblings are not, and the small int<->float conversions are
// costed while the 64-bit ones are not -- this tracks each routine's soundness
// regime, not its family.
static const llvm::StringMap<unsigned> &mspabiComputeCosts() {
  static const llvm::StringMap<unsigned> Costs = {
      // Integer 16/32-bit (original set)
      {"__mspabi_mpyi", 304},
      {"__mspabi_divi", 451},
      {"__mspabi_divu", 443},
      {"__mspabi_remi", 468},
      {"__mspabi_remu", 444},
      {"__mspabi_slli", 89},
      {"__mspabi_srai", 89},
      {"__mspabi_srli", 104},
      {"__mspabi_mpyl", 981},
      {"__mspabi_divli", 1624},
      {"__mspabi_divlu", 1601},
      {"__mspabi_remli", 1840},
      {"__mspabi_remul", 1603},
      // Integer 64-bit (only remlli is `ok`)
      {"__mspabi_remlli", 9614},
      // Variable shifts, 32/64-bit
      {"__mspabi_slll", 215},
      {"__mspabi_sral", 215},
      {"__mspabi_srll", 246},
      {"__mspabi_sllll", 508},
      {"__mspabi_srall", 508},
      {"__mspabi_srlll", 563},
      // int -> float / double (small; 64-bit sources are no-static)
      {"__mspabi_fltif", 527},
      {"__mspabi_fltuf", 500},
      {"__mspabi_fltlif", 515},
      {"__mspabi_fltulf", 491},
      {"__mspabi_fltid", 867},
      {"__mspabi_fltud", 838},
      {"__mspabi_fltlid", 855},
      {"__mspabi_fltuld", 829},
      // float/double -> int, and float <-> double (small; int64 dests are
      // no-static)
      {"__mspabi_fixfli", 485},
      {"__mspabi_fixdli", 2683},
      {"__mspabi_cvtfd", 1047},
      {"__mspabi_cvtdf", 3179},
  };
  return Costs;
}

std::optional<unsigned>
MSP430FR5994Target::getExternalCallCost(llvm::StringRef CalleeName) const {
  // The measured `compute` bound holds only at NWAITS=0 (RAM-resident/<=8 MHz).
  // At any FRAM wait state the per-call fetch penalty is layout-dependent and
  // has no portable upper bound, so we return nullopt and the call stays
  // flagged UNSOUND rather than emit an unsound under-approximation.
  if (FRAMWaitStates != 0)
    return std::nullopt;
  const llvm::StringMap<unsigned> &Costs = mspabiComputeCosts();
  auto It = Costs.find(CalleeName);
  if (It == Costs.end())
    return std::nullopt; // no-static FP/64-bit / libm / unlisted -> UNSOUND
  return It->second;
}

} // namespace llta
