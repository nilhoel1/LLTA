#ifndef ANALYSIS_CACHE_CACHE_ACCESS_MAPPER_H
#define ANALYSIS_CACHE_CACHE_ACCESS_MAPPER_H

#include "Analysis/Cache/CacheEvent.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

class MachineInstr;

/// Maps a MachineInstr to the ordered sequence of cache effects it produces.
///
/// This is the target-/cache-specific extension point of the modular cache
/// analysis: the generic engine (CacheAnalysis) and the replacement-policy
/// module are reused unchanged; only the mapper changes per target. For
/// example FRAMAccessMapper emits one Access per FRAM fetch word plus a Barrier
/// for non-stack data accesses.
class CacheAccessMapper {
public:
  virtual ~CacheAccessMapper() = default;

  /// Append the cache events of \p MI, in program (access) order, to \p Out.
  virtual void mapEvents(const MachineInstr *MI,
                         SmallVectorImpl<CacheEvent> &Out) = 0;
};

} // namespace llvm

#endif // ANALYSIS_CACHE_CACHE_ACCESS_MAPPER_H
