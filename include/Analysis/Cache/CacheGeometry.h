#ifndef ANALYSIS_CACHE_CACHE_GEOMETRY_H
#define ANALYSIS_CACHE_CACHE_GEOMETRY_H

#include <cstdint>

namespace llvm {

/// Geometry of a set-associative cache, shared by every cache analysis.
///
/// Target-agnostic: it carries only the structural parameters and the
/// address-decomposition helpers. `NumSets` and `LineBytes` must be powers of
/// two (the usual case), so set selection is a mask rather than a division.
struct CacheGeometry {
  unsigned NumSets = 1;
  unsigned Ways = 1;
  unsigned LineBytes = 8;

  CacheGeometry() = default;
  CacheGeometry(unsigned NumSets, unsigned Ways, unsigned LineBytes)
      : NumSets(NumSets), Ways(Ways), LineBytes(LineBytes) {}

  /// Base address of the cache line containing \p Addr (the line identity).
  uint64_t lineId(uint64_t Addr) const {
    return Addr & ~(static_cast<uint64_t>(LineBytes) - 1);
  }

  /// Index of the set a line maps to. Adjacent lines alternate across sets.
  unsigned setIndex(uint64_t LineId) const {
    return static_cast<unsigned>((LineId / LineBytes) & (NumSets - 1));
  }

  /// True if the parameters are a usable power-of-two geometry.
  bool isValid() const {
    auto isPow2 = [](unsigned X) { return X != 0 && (X & (X - 1)) == 0; };
    return isPow2(NumSets) && isPow2(LineBytes) && Ways >= 1;
  }
};

} // namespace llvm

#endif // ANALYSIS_CACHE_CACHE_GEOMETRY_H
