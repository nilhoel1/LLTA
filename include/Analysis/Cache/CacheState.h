#ifndef ANALYSIS_CACHE_CACHE_STATE_H
#define ANALYSIS_CACHE_CACHE_STATE_H

#include "Analysis/AbstractState.h"
#include "Analysis/Cache/CacheGeometry.h"
#include "Analysis/Cache/ReplacementPolicy.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace llvm {

/// Generic abstract cache state: one per-set CacheSetState (owned and
/// interpreted by the ReplacementPolicy) per cache set, plus the analysis
/// direction (Must/May, which selects the join). Fully cache-/policy-agnostic;
/// every lattice operation delegates to the policy.
class CacheState : public AbstractState {
public:
  CacheState(CacheGeometry Geo, const ReplacementPolicy *Policy,
             AnalysisKind Kind)
      : Geo(Geo), Policy(Policy), Kind(Kind) {
    Sets.reserve(Geo.NumSets);
    for (unsigned I = 0; I < Geo.NumSets; ++I)
      Sets.push_back(Policy->makeEmpty());
  }

  /// Apply an access to line \p LineId; return whether the line was present
  /// (Must: a guaranteed hit; May: a possible hit, i.e. not a guaranteed miss).
  bool access(uint64_t LineId) {
    unsigned S = Geo.setIndex(LineId);
    bool Present = Policy->contains(*Sets[S], LineId);
    Policy->update(*Sets[S], LineId);
    return Present;
  }

  /// Conservatively wipe the whole cache (an unplaceable/unknown access).
  void barrier() {
    for (auto &S : Sets)
      S = Policy->makeEmpty();
  }

  std::unique_ptr<AbstractState> clone() const override {
    auto C = std::make_unique<CacheState>(Geo, Policy, Kind);
    for (unsigned I = 0; I < Sets.size(); ++I)
      C->Sets[I] = Policy->clone(*Sets[I]);
    return C;
  }

  bool equals(const AbstractState *Other) const override {
    const auto *O = static_cast<const CacheState *>(Other);
    if (Sets.size() != O->Sets.size())
      return false;
    for (unsigned I = 0; I < Sets.size(); ++I)
      if (!Policy->equals(*Sets[I], *O->Sets[I]))
        return false;
    return true;
  }

  bool join(const AbstractState *Other) override {
    const auto *O = static_cast<const CacheState *>(Other);
    bool Changed = false;
    for (unsigned I = 0; I < Sets.size() && I < O->Sets.size(); ++I)
      if (Policy->join(*Sets[I], *O->Sets[I], Kind))
        Changed = true;
    return Changed;
  }

  std::string toString() const override {
    std::string Res = "Cache[";
    for (unsigned I = 0; I < Sets.size(); ++I) {
      if (I)
        Res += " ";
      Res += Policy->toString(*Sets[I]);
    }
    return Res + "]";
  }

private:
  CacheGeometry Geo;
  const ReplacementPolicy *Policy;
  AnalysisKind Kind;
  std::vector<std::unique_ptr<CacheSetState>> Sets;
};

} // namespace llvm

#endif // ANALYSIS_CACHE_CACHE_STATE_H
