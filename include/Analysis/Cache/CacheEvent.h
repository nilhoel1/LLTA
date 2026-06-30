#ifndef ANALYSIS_CACHE_CACHE_EVENT_H
#define ANALYSIS_CACHE_CACHE_EVENT_H

#include <cstdint>

namespace llvm {

/// One memory effect an instruction has on the cache, produced by a
/// CacheAccessMapper and consumed by the CacheAnalysis engine.
///
///   Access  — a read of the cached line at `LineId` (a line base address).
///             The engine classifies it hit/miss and updates the abstract
///             state via the replacement policy.
///   Barrier — an access the analysis cannot place (e.g. an FRAM data read at
///             an unknown address, or any other event that may perturb the
///             cache). The engine conservatively wipes the abstract state, and
///             charges `Cost` cycles (the FRAM data-access wait-state penalty)
///             in the must (WCET) direction.
struct CacheEvent {
  enum Kind { Access, Barrier };
  Kind Kind;
  uint64_t LineId; ///< valid only when Kind == Access
  unsigned Cost;   ///< extra WCET cycles to charge; valid only when Kind == Barrier

  static CacheEvent access(uint64_t LineId) { return {Access, LineId, 0}; }
  static CacheEvent barrier(unsigned Cost = 0) { return {Barrier, 0, Cost}; }
};

} // namespace llvm

#endif // ANALYSIS_CACHE_CACHE_EVENT_H
