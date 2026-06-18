#ifndef ANALYSIS_CACHE_REPLACEMENT_POLICY_H
#define ANALYSIS_CACHE_REPLACEMENT_POLICY_H

#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace llvm {

/// Which direction of abstract cache analysis a state/operation belongs to.
///   Must — guaranteed-present lines; join is intersection (max age). A present
///          line is an always-hit.
///   May  — possibly-present lines; join is union (min age). An absent line is
///          an always-miss.
enum class AnalysisKind { Must, May };

//===----------------------------------------------------------------------===//
// Replacement-policy modules
//
// A ReplacementPolicy is the plug-and-play core of the modular cache analysis.
// It owns the abstract state of a *single* cache set (an opaque CacheSetState it
// sub-classes) and defines, for that domain, the membership test (contains),
// the transfer function (update), the meet operator (join, per AnalysisKind),
// equality and cloning. The same policy serves both the must- and may-analysis;
// only join and the engine's interpretation of `contains` differ by kind.
//
// Shipped modules:
//   UnknownPolicy — adversarial; sound (must) for ANY real policy. Must-only.
//   LRUPolicy     — age-based, reorder-on-hit. Supports must and may.
//   FIFOPolicy    — age-based, NO reorder-on-hit. Supports must and may.
//
// Adding PLRUPolicy etc. is just another subclass.
//===----------------------------------------------------------------------===//

/// Opaque per-set abstract state; each policy sub-classes this.
class CacheSetState {
public:
  virtual ~CacheSetState() = default;
};

class ReplacementPolicy {
public:
  virtual ~ReplacementPolicy() = default;

  virtual StringRef name() const = 0;

  /// True if this policy defines the given analysis direction.
  virtual bool supports(AnalysisKind Kind) const = 0;

  /// An empty (cold) set state.
  virtual std::unique_ptr<CacheSetState> makeEmpty() const = 0;
  virtual std::unique_ptr<CacheSetState> clone(const CacheSetState &S) const = 0;
  virtual bool equals(const CacheSetState &A, const CacheSetState &B) const = 0;

  /// Membership test (no mutation): is \p LineId in the abstract set? For a
  /// must-state this means "guaranteed present" (⇒ hit); for a may-state
  /// "possibly present" (absent ⇒ guaranteed miss).
  virtual bool contains(const CacheSetState &S, uint64_t LineId) const = 0;

  /// Apply the policy's transfer for an access to \p LineId (insert / age /
  /// evict). Same mechanics for must and may.
  virtual void update(CacheSetState &S, uint64_t LineId) const = 0;

  /// Meet \p Other into \p Into for the given analysis direction. Returns true
  /// iff \p Into changed.
  virtual bool join(CacheSetState &Into, const CacheSetState &Other,
                    AnalysisKind Kind) const = 0;

  virtual std::string toString(const CacheSetState &S) const = 0;
};

//===----------------------------------------------------------------------===//
// UnknownPolicy — adversarial replacement (sound must default)
//
// Under an undocumented/adversarial policy the only line provably resident in a
// set is the most-recently-accessed one, with no intervening distinct-line
// access (a hit evicts nothing; only an insertion can evict, and an adversary
// could evict any other resident line). So the must-state of a set is a single
// optional line. This is exactly the must-analysis of a direct-mapped (1-way)
// cache and a subset of the real k-way guaranteed hits — hence sound for any
// real policy. It has no useful may form (a single line cannot over-approximate
// the up-to-k lines a real cache may hold), so it is must-only; the may-analysis
// uses a concrete age-based policy instead.
//===----------------------------------------------------------------------===//

class UnknownSetState : public CacheSetState {
public:
  std::optional<uint64_t> Line; ///< the single guaranteed-resident line, if any
};

class UnknownPolicy : public ReplacementPolicy {
public:
  StringRef name() const override { return "unknown"; }
  bool supports(AnalysisKind Kind) const override {
    return Kind == AnalysisKind::Must;
  }

  std::unique_ptr<CacheSetState> makeEmpty() const override {
    return std::make_unique<UnknownSetState>();
  }
  std::unique_ptr<CacheSetState> clone(const CacheSetState &S) const override {
    return std::make_unique<UnknownSetState>(static_cast<const UnknownSetState &>(S));
  }
  bool equals(const CacheSetState &A, const CacheSetState &B) const override {
    return static_cast<const UnknownSetState &>(A).Line ==
           static_cast<const UnknownSetState &>(B).Line;
  }
  bool contains(const CacheSetState &S, uint64_t LineId) const override {
    const auto &U = static_cast<const UnknownSetState &>(S);
    return U.Line.has_value() && *U.Line == LineId;
  }
  void update(CacheSetState &S, uint64_t LineId) const override {
    static_cast<UnknownSetState &>(S).Line = LineId; // now definitely resident
  }
  bool join(CacheSetState &Into, const CacheSetState &Other,
            AnalysisKind /*Kind*/) const override {
    // Must-only policy: intersection of the single guaranteed lines.
    auto &I = static_cast<UnknownSetState &>(Into);
    const auto &O = static_cast<const UnknownSetState &>(Other);
    std::optional<uint64_t> Merged =
        (I.Line && O.Line && *I.Line == *O.Line) ? I.Line : std::nullopt;
    if (Merged == I.Line)
      return false;
    I.Line = Merged;
    return true;
  }
  std::string toString(const CacheSetState &S) const override {
    const auto &U = static_cast<const UnknownSetState &>(S);
    return U.Line ? ("{" + std::to_string(*U.Line) + "}") : "{}";
  }
};

//===----------------------------------------------------------------------===//
// Age-based policies (LRU, FIFO)
//
// Both track, per set, the lines present with an upper/lower bound on their age
// in [0, Ways-1] (upper bound for must, lower bound for may; the representation
// is shared, only join differs). A line is present iff it appears. They differ
// only in the transfer on a *hit*: LRU moves the line to MRU (reorder), FIFO
// leaves the order untouched. Everything else (insertion, ageing, eviction,
// join, membership) is shared in AgeBasedPolicy.
//===----------------------------------------------------------------------===//

class AgeSetState : public CacheSetState {
public:
  std::vector<std::pair<uint64_t, unsigned>> Lines; ///< (line, age bound)
};

class AgeBasedPolicy : public ReplacementPolicy {
public:
  explicit AgeBasedPolicy(unsigned Ways) : Ways(Ways == 0 ? 1 : Ways) {}

  bool supports(AnalysisKind) const override { return true; }

  std::unique_ptr<CacheSetState> makeEmpty() const override {
    return std::make_unique<AgeSetState>();
  }
  std::unique_ptr<CacheSetState> clone(const CacheSetState &S) const override {
    return std::make_unique<AgeSetState>(static_cast<const AgeSetState &>(S));
  }
  bool equals(const CacheSetState &A, const CacheSetState &B) const override {
    return sorted(static_cast<const AgeSetState &>(A)) ==
           sorted(static_cast<const AgeSetState &>(B));
  }
  bool contains(const CacheSetState &S, uint64_t LineId) const override {
    const auto &L = static_cast<const AgeSetState &>(S).Lines;
    return std::any_of(L.begin(), L.end(),
                       [&](const auto &P) { return P.first == LineId; });
  }
  bool join(CacheSetState &Into, const CacheSetState &Other,
            AnalysisKind Kind) const override {
    const auto &I = static_cast<AgeSetState &>(Into).Lines;
    const auto &O = static_cast<const AgeSetState &>(Other).Lines;
    std::vector<std::pair<uint64_t, unsigned>> Merged;
    if (Kind == AnalysisKind::Must) {
      // Intersection, oldest (max) age.
      for (const auto &P : I)
        if (const unsigned *AO = find(O, P.first))
          Merged.push_back({P.first, std::max(P.second, *AO)});
    } else {
      // Union, youngest (min) age.
      Merged = I;
      for (auto &P : Merged)
        if (const unsigned *AO = find(O, P.first))
          P.second = std::min(P.second, *AO);
      for (const auto &Q : O)
        if (!find(I, Q.first))
          Merged.push_back(Q);
    }
    AgeSetState Tmp;
    Tmp.Lines = Merged;
    if (equals(Into, Tmp))
      return false;
    static_cast<AgeSetState &>(Into).Lines = std::move(Merged);
    return true;
  }
  std::string toString(const CacheSetState &S) const override {
    auto C = sorted(static_cast<const AgeSetState &>(S));
    std::string Res = "{";
    for (size_t I = 0; I < C.size(); ++I) {
      if (I)
        Res += ",";
      Res += std::to_string(C[I].first) + "@" + std::to_string(C[I].second);
    }
    return Res + "}";
  }

protected:
  unsigned Ways;

  /// Insert \p LineId at MRU (age 0), age every present line, evict on overflow.
  void insertMiss(AgeSetState &S, uint64_t LineId) const {
    for (auto &P : S.Lines)
      ++P.second;
    S.Lines.erase(std::remove_if(S.Lines.begin(), S.Lines.end(),
                                 [&](const auto &P) { return P.second >= Ways; }),
                  S.Lines.end());
    S.Lines.push_back({LineId, 0});
  }

  static const unsigned *
  find(const std::vector<std::pair<uint64_t, unsigned>> &V, uint64_t LineId) {
    for (const auto &P : V)
      if (P.first == LineId)
        return &P.second;
    return nullptr;
  }
  static std::vector<std::pair<uint64_t, unsigned>>
  sorted(const AgeSetState &S) {
    auto C = S.Lines;
    std::sort(C.begin(), C.end());
    return C;
  }
};

/// LRU: a hit moves the line to MRU and ages the lines that were younger.
class LRUPolicy : public AgeBasedPolicy {
public:
  explicit LRUPolicy(unsigned Ways) : AgeBasedPolicy(Ways) {}
  StringRef name() const override { return "lru"; }

  void update(CacheSetState &S, uint64_t LineId) const override {
    auto &L = static_cast<AgeSetState &>(S);
    for (auto &P : L.Lines) {
      if (P.first == LineId) { // hit: move to MRU, age the younger lines
        unsigned A = P.second;
        for (auto &Q : L.Lines)
          if (Q.second < A)
            ++Q.second;
        P.second = 0;
        return;
      }
    }
    insertMiss(L, LineId); // miss
  }
};

/// FIFO: a hit does NOT reorder; age reflects insertion order only.
class FIFOPolicy : public AgeBasedPolicy {
public:
  explicit FIFOPolicy(unsigned Ways) : AgeBasedPolicy(Ways) {}
  StringRef name() const override { return "fifo"; }

  void update(CacheSetState &S, uint64_t LineId) const override {
    auto &L = static_cast<AgeSetState &>(S);
    if (contains(L, LineId))
      return;          // hit: FIFO leaves the queue untouched
    insertMiss(L, LineId); // miss: enqueue, evict oldest on overflow
  }
};

} // namespace llvm

#endif // ANALYSIS_CACHE_REPLACEMENT_POLICY_H
