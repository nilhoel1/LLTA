#ifndef ANALYSIS_CACHE_CACHE_ANALYSIS_H
#define ANALYSIS_CACHE_CACHE_ANALYSIS_H

#include "Analysis/AbstractAnalysable.h"
#include "Analysis/Cache/CacheAccessMapper.h"
#include "Analysis/Cache/CacheGeometry.h"
#include "Analysis/Cache/ReplacementPolicy.h"

#include "llvm/CodeGen/MachineInstr.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace llvm {

/// Generic cache analysis engine (an AbstractAnalysable for the LLTA
/// abstract-interpretation framework / WorklistSolver).
///
/// Assembled from three interchangeable parts, with no target-specific logic:
///   - a CacheGeometry,
///   - a ReplacementPolicy (the must-/may-domain and hit rule),
///   - a CacheAccessMapper (what accesses each instruction makes),
/// plus an AnalysisKind selecting the direction:
///   - Must: each Access not provably a hit costs MissPenalty cycles (a sound
///     WCET fetch-penalty model).
///   - May: cost is always 0; each Access that is provably *not* cached (a
///     guaranteed miss) is reported through the optional DefiniteMissSink — a
///     diagnostic, not a WCET contribution.
///
/// A new cache analysis is built by supplying a different mapper/policy/geometry
/// — no engine change.
class CacheAnalysis : public AbstractAnalysable {
public:
  /// Called for each guaranteed-miss access in May mode (if set).
  using DefiniteMissSink =
      std::function<void(const MachineInstr *MI, uint64_t LineId)>;

  CacheAnalysis(CacheGeometry Geo, unsigned MissPenalty,
                const ReplacementPolicy &Policy, CacheAccessMapper &Mapper,
                AnalysisKind Kind)
      : Geo(Geo), MissPenalty(MissPenalty), Policy(&Policy), Mapper(&Mapper),
        Kind(Kind) {}

  /// Enable/disable diagnostic reporting of guaranteed misses (May mode only).
  /// Leave unset during the fixpoint; set it for a final replay pass.
  void setDefiniteMissSink(DefiniteMissSink Sink) { this->Sink = std::move(Sink); }

  std::unique_ptr<AbstractState> getInitialState() override;

  unsigned process(AbstractState *State, const MachineInstr *MI) override;

private:
  CacheGeometry Geo;
  unsigned MissPenalty;
  const ReplacementPolicy *Policy;
  CacheAccessMapper *Mapper;
  AnalysisKind Kind;
  DefiniteMissSink Sink;
};

} // namespace llvm

#endif // ANALYSIS_CACHE_CACHE_ANALYSIS_H
