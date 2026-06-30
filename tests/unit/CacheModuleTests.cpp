//===- CacheModuleTests.cpp - unit tests for the modular cache analysis ---===//
//
// A dependency-light standalone test binary (no GoogleTest) exercising the
// header-only cache modules in include/Analysis/Cache/:
//   - CacheGeometry address decomposition,
//   - the replacement-policy modules (UnknownPolicy / LRUPolicy / FIFOPolicy),
//     in both must- and may-analysis directions,
//   - the generic CacheState (multi-set, barrier, join).
//
// The FRAMAccessMapper needs a live MachineInstr (covered by the
// MachineFunctionGraphTests framDataAccessWords test and the end-to-end run),
// but the CacheAnalysis cost engine is exercised here with a stub mapper that
// emits chosen events, so the miss/data-access charging is locked in directly.
//
// Run via CTest (`ctest -R LLTACacheModuleTests`) or the `check-llta-cache`
// build target. Exits non-zero if any check fails.
//===----------------------------------------------------------------------===//

#include "Analysis/Cache/CacheAccessMapper.h"
#include "Analysis/Cache/CacheAnalysis.h"
#include "Analysis/Cache/CacheEvent.h"
#include "Analysis/Cache/CacheGeometry.h"
#include "Analysis/Cache/CacheState.h"
#include "Analysis/Cache/ReplacementPolicy.h"

#include <iostream>
#include <memory>
#include <string>

using namespace llvm;

static int Checks = 0;
static int Failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++Checks;                                                                  \
    if (!(cond)) {                                                             \
      ++Failures;                                                              \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #cond   \
                << "\n";                                                       \
    }                                                                          \
  } while (0)

#define CHECK_EQ(a, b)                                                         \
  do {                                                                         \
    ++Checks;                                                                  \
    auto Va = (a);                                                             \
    auto Vb = (b);                                                             \
    if (!(Va == Vb)) {                                                         \
      ++Failures;                                                              \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #a      \
                << " == " << #b << "  (" << Va << " != " << Vb << ")\n";       \
    }                                                                          \
  } while (0)

// Apply an access to a single set state via a policy; return whether the line
// was present *before* the update (must: guaranteed hit; may: possible hit).
static bool touch(const ReplacementPolicy &P, CacheSetState &S, uint64_t Line) {
  bool Present = P.contains(S, Line);
  P.update(S, Line);
  return Present;
}

static void testGeometry() {
  CacheGeometry G(/*sets=*/2, /*ways=*/2, /*line=*/8);
  CHECK(G.isValid());
  // Line base alignment.
  CHECK_EQ(G.lineId(0x4000u), 0x4000u);
  CHECK_EQ(G.lineId(0x4003u), 0x4000u);
  CHECK_EQ(G.lineId(0x4007u), 0x4000u);
  CHECK_EQ(G.lineId(0x4008u), 0x4008u);
  // Adjacent 8-byte lines alternate sets (set = address bit 3).
  CHECK_EQ(G.setIndex(0x4000u), 0u);
  CHECK_EQ(G.setIndex(0x4008u), 1u);
  CHECK_EQ(G.setIndex(0x4010u), 0u);

  // Non-power-of-two geometries are rejected.
  CHECK(!CacheGeometry(3, 2, 8).isValid());
  CHECK(!CacheGeometry(2, 2, 6).isValid());
}

static void testUnknownPolicy() {
  UnknownPolicy P;
  CHECK(P.supports(AnalysisKind::Must));
  CHECK(!P.supports(AnalysisKind::May));
  auto S = P.makeEmpty();

  // Adversarial: a hit only on immediate re-access of the same line.
  CHECK(!touch(P, *S, 100)); // cold miss
  CHECK(touch(P, *S, 100));  // re-access -> guaranteed hit
  CHECK(!touch(P, *S, 200)); // distinct line -> miss
  CHECK(!touch(P, *S, 100)); // 100 no longer guaranteed (single-line set)

  // Must-join is intersection of the single guaranteed lines.
  auto A = P.makeEmpty();
  auto B = P.makeEmpty();
  P.update(*A, 7);
  P.update(*B, 7);
  CHECK(!P.join(*A, *B, AnalysisKind::Must)); // equal -> unchanged
  CHECK(P.contains(*A, 7));
  P.update(*B, 9);                           // B now {9}
  CHECK(P.join(*A, *B, AnalysisKind::Must)); // differ -> drop to empty
  CHECK(!P.contains(*A, 7));
}

static void testLRUPolicy() {
  LRUPolicy P(/*ways=*/2);
  auto S = P.makeEmpty();
  // Sequence 1,2,1,3 in a 2-way set. The hit on 1 keeps it MRU, so 3 evicts 2.
  CHECK(!touch(P, *S, 1));
  CHECK(!touch(P, *S, 2));
  CHECK(touch(P, *S, 1)); // hit, moves 1 to MRU
  CHECK(!touch(P, *S, 3)); // miss, evicts LRU = 2
  CHECK(P.contains(*S, 1));
  CHECK(!P.contains(*S, 2)); // evicted
  CHECK(P.contains(*S, 3));

  // Must-join: intersection, oldest (max) age.  May-join: union, youngest age.
  auto mkP = [&] { auto s = P.makeEmpty(); P.update(*s, 2); P.update(*s, 1); return s; }; // {1@0,2@1}
  auto mkQ = [&] { auto s = P.makeEmpty(); P.update(*s, 3); P.update(*s, 1); return s; }; // {1@0,3@1}
  auto M = mkP();
  CHECK(P.join(*M, *mkQ(), AnalysisKind::Must));
  CHECK_EQ(P.toString(*M), std::string("{1@0}")); // only 1 in both, max age 0
  auto Y = mkP();
  CHECK(P.join(*Y, *mkQ(), AnalysisKind::May));
  CHECK_EQ(P.toString(*Y), std::string("{1@0,2@1,3@1}")); // union, min ages
}

static void testFIFOPolicy() {
  FIFOPolicy P(/*ways=*/2);
  auto S = P.makeEmpty();
  // Same 1,2,1,3 sequence. FIFO ignores the hit on 1, so 1 (oldest) is evicted.
  CHECK(!touch(P, *S, 1));
  CHECK(!touch(P, *S, 2));
  CHECK(touch(P, *S, 1));  // hit, but FIFO does NOT reorder
  CHECK(!touch(P, *S, 3)); // miss, evicts oldest-by-insertion = 1
  CHECK(!P.contains(*S, 1)); // evicted (contrast: LRU kept 1)
  CHECK(P.contains(*S, 2));
  CHECK(P.contains(*S, 3));
}

static void testCacheState() {
  CacheGeometry G(/*sets=*/2, /*ways=*/2, /*line=*/8);
  UnknownPolicy P;
  CacheState C(G, &P, AnalysisKind::Must);

  // Lines in different sets are independent (0x4000 -> set0, 0x4008 -> set1).
  CHECK(!C.access(0x4000)); // miss
  CHECK(!C.access(0x4008)); // miss, different set
  CHECK(C.access(0x4000));  // still resident in set0 -> hit
  CHECK(C.access(0x4008));  // still resident in set1 -> hit

  // Barrier wipes everything.
  C.barrier();
  CHECK(!C.access(0x4000)); // cold again after barrier

  // clone is independent.
  CHECK(!C.access(0x4010)); // 0x4010 -> set0, miss (fills set0)
  auto Clone = C.clone();
  CHECK(static_cast<CacheState *>(Clone.get())->access(0x4010)); // hit in clone
  // Mutating the clone (distinct line, same set) must not affect the original.
  CHECK(!static_cast<CacheState *>(Clone.get())->access(0x4020));
  CHECK(C.access(0x4010)); // original still has 0x4010 resident -> hit
}

// A mapper that ignores the (possibly null) MachineInstr and emits a fixed
// event list, so the CacheAnalysis cost engine can be unit-tested without a
// live MachineInstr.
namespace {
class StubMapper : public CacheAccessMapper {
public:
  SmallVector<CacheEvent, 4> Events;
  void mapEvents(const MachineInstr *, SmallVectorImpl<CacheEvent> &Out) override {
    Out.append(Events.begin(), Events.end());
  }
};
} // namespace

// (a) A fetch miss costs the full line-fill penalty (MissPenalty), once per
// line; a subsequent access to the now-resident line is a free hit.
static void testMissCostsLineFill() {
  CacheGeometry G(/*sets=*/2, /*ways=*/2, /*line=*/8);
  LRUPolicy P(/*ways=*/2);
  StubMapper M;
  M.Events = {CacheEvent::access(0x4000)};
  CacheAnalysis A(G, /*MissPenalty=*/15, P, M, AnalysisKind::Must);
  auto S = A.getInitialState();
  CHECK_EQ(A.process(S.get(), nullptr), 15u); // cold miss -> full line fill
  CHECK_EQ(A.process(S.get(), nullptr), 0u);  // resident -> hit, no penalty
}

// (b) An unproven/FRAM data access (a Barrier carrying the wait-state cost) is
// charged in the must (WCET) direction and costs nothing in the may direction.
static void testDataAccessCharged() {
  CacheGeometry G(/*sets=*/2, /*ways=*/2, /*line=*/8);
  LRUPolicy P(/*ways=*/2);
  StubMapper M;
  M.Events = {CacheEvent::barrier(/*Cost=*/3)};

  CacheAnalysis Must(G, /*MissPenalty=*/15, P, M, AnalysisKind::Must);
  auto SM = Must.getInitialState();
  CHECK_EQ(Must.process(SM.get(), nullptr), 3u); // data wait state charged

  CacheAnalysis May(G, /*MissPenalty=*/15, P, M, AnalysisKind::May);
  auto SY = May.getInitialState();
  CHECK_EQ(May.process(SY.get(), nullptr), 0u); // may direction: never a cost
}

// (c) Model-off shape: a zero line-fill penalty and zero-cost barrier add
// nothing, mirroring the disabled-model knobs.
static void testModelOffShapeIsZero() {
  CacheGeometry G(/*sets=*/2, /*ways=*/2, /*line=*/8);
  LRUPolicy P(/*ways=*/2);
  StubMapper M;
  M.Events = {CacheEvent::access(0x4000), CacheEvent::barrier(/*Cost=*/0)};
  CacheAnalysis A(G, /*MissPenalty=*/0, P, M, AnalysisKind::Must);
  auto S = A.getInitialState();
  CHECK_EQ(A.process(S.get(), nullptr), 0u);
}

int main() {
  testGeometry();
  testUnknownPolicy();
  testLRUPolicy();
  testFIFOPolicy();
  testCacheState();
  testMissCostsLineFill();
  testDataAccessCharged();
  testModelOffShapeIsZero();

  if (Failures == 0) {
    std::cout << "All " << Checks << " checks passed.\n";
    return 0;
  }
  std::cerr << Failures << " of " << Checks << " checks FAILED.\n";
  return 1;
}
