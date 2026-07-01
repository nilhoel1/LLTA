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

// Measured worst-case cycle bounds for the body-less integer __mspabi_*
// soft-arithmetic helpers, from lib/Targets/MSP430/ABI_FINDINGS.md
// (MSP-EXP430FR5994 Launchpad, 2026-06-23, Timer_B0 single-call method,
// cross-checked measured <= the sound static ceiling from
// scripts/derive_abi_costs.py). Each entry is the worst-case callee *body* cost
// at that routine's worst-case input; the `call` instruction itself is costed
// separately at the call site. Float/double helpers and libm are deliberately
// absent: their worst case is not measurable here (unresolvable indirect call
// in the soft-float tree -- see docs/LibraryCallCosting.md), so they stay
// UNSOUND. The 32-bit shifts (__mspabi_sral/srll/slll) are likewise unmeasured.

// compute: 8 MHz, NWAITS=0 -- the bound for a RAM-resident or <=8 MHz
// deployment (-fram-wait-states=0). Cache- and placement-independent.
static const llvm::StringMap<unsigned> &mspabiComputeCosts() {
  static const llvm::StringMap<unsigned> Costs = {
      {"__mspabi_mpyi", 304},   {"__mspabi_divi", 451},
      {"__mspabi_divu", 443},   {"__mspabi_remi", 468},
      {"__mspabi_remu", 444},   {"__mspabi_slli", 89},
      {"__mspabi_srai", 89},    {"__mspabi_srli", 104},
      {"__mspabi_mpyl", 981},   {"__mspabi_divli", 1624},
      {"__mspabi_divlu", 1601}, {"__mspabi_remli", 1840},
      {"__mspabi_remul", 1603},
  };
  return Costs;
}

// fram_tot: 16 MHz, NWAITS=1, cold cache -- compute plus the worst-case
// instruction-fetch penalty for the default FRAM-resident deployment
// (-fram-wait-states=1). Larger than compute because the big routines' loop
// bodies exceed the FRAM cache and re-miss every iteration.
static const llvm::StringMap<unsigned> &mspabiFramTotCosts() {
  static const llvm::StringMap<unsigned> Costs = {
      {"__mspabi_mpyi", 919},   {"__mspabi_divi", 2086},
      {"__mspabi_divu", 2003},  {"__mspabi_remi", 2103},
      {"__mspabi_remu", 2004},  {"__mspabi_slli", 134},
      {"__mspabi_srai", 119},   {"__mspabi_srli", 134},
      {"__mspabi_mpyl", 4911},  {"__mspabi_divli", 9934},
      {"__mspabi_divlu", 9866}, {"__mspabi_remli", 10240},
      {"__mspabi_remul", 9868},
  };
  return Costs;
}

std::optional<unsigned>
MSP430FR5994Target::getExternalCallCost(llvm::StringRef CalleeName) const {
  // Pick the measured deployment column matching the FRAM fetch model. Only
  // NWAITS 0 and 1 were measured; any other value is unmeasured, so we return
  // nullopt and the call stays flagged UNSOUND rather than emit an unsound
  // under-approximation.
  const llvm::StringMap<unsigned> *Table = nullptr;
  switch (FRAMWaitStates) {
  case 0:
    Table = &mspabiComputeCosts();
    break;
  case 1:
    Table = &mspabiFramTotCosts();
    break;
  default:
    return std::nullopt;
  }
  auto It = Table->find(CalleeName);
  if (It == Table->end())
    return std::nullopt; // float/double/libm/unmeasured -> stays UNSOUND
  return It->second;
}

} // namespace llta
