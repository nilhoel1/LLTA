//===- ILPSolverTests.cpp - unit tests for the WCET ILP formulation -------===//
//
// A dependency-light standalone test binary (no GoogleTest) that exercises the
// IPET/ILP formulation in lib/ILP/ in isolation, without the MSP430 toolchain.
//
// The solver consumes an AbstractStateGraph (ASG) and reads only:
//   - Node->Cost, Node->IsEntry/IsExit, Node->IsLoopHeader,
//   Node->UpperLoopBound
//   - edges (with their IsBackEdge flag)
//   - CallSites / FunctionEntries / FunctionReturns
// It never dereferences Node->State, so each test builds an ASG by hand
// (passing a null AbstractState) and calls AbstractHighsSolver::solveWCET,
// asserting the resulting WCET / Status.
//
// IPET flow semantics (used to derive the expected numbers below): with the
// entry constraint x_entry = 1, flow conservation x_u = sum(in) = sum(out), and
// the loop-bound row (B-1)*x_header - B*sum(backedges) >= 0, a loop with bound
// N executes its header N times and its body N-1 times (the backedge is taken
// at most N-1 times).
//
// One case is a *documented expected-failure* (CHECK_GAP): an unbounded loop
// (UpperLoopBound==0). It asserts the CURRENT (limited) behavior -- the solver
// reports a non-empty Status (no WCET) -- and prints a GAP line. The day the
// implementation learns to bound it, the assertion flips to red, the intended
// "update me" signal. (Self-recursion was finding #5's GAP; it is now bounded
// and covered by testRecursionBounded.)
//
// HiGHS is the always-available open-source backend. When the build has no ILP
// backend enabled (ENABLE_HIGHS undefined for this target) the tests are
// skipped with a success exit, because solveWCET cannot produce a result.
//
// Run via CTest (`ctest -R LLTAILPSolverTests`) or the `check-llta-ilp` target.
//===----------------------------------------------------------------------===//

#include "Analysis/AbstractStateGraph.h" // pulls in AbstractState.h
#include "ILP/AbstractHighsSolver.h"

#include <cmath>
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

// Documented expected-failure: assert the CURRENT (limited) behavior and print
// a GAP note. When the gap is later closed, `cond` becomes false and the test
// turns red, signalling that this assertion must be updated to the new
// contract.
#define CHECK_GAP(cond, note)                                                  \
  do {                                                                         \
    std::cout << "GAP: " << note << "\n";                                      \
    CHECK(cond);                                                               \
  } while (0)

#ifdef ENABLE_HIGHS

// WCET is returned as a double; compare against an exact integer expectation.
static bool wcetEq(double W, long Expected) {
  return std::llround(W) == Expected;
}

// addNode takes ownership of an AbstractState; the solver never dereferences
// it, so a null state is sufficient for these formulation tests.
static unsigned addNode(AbstractStateGraph &G, unsigned Cost,
                        bool IsEntry = false, bool IsExit = false) {
  unsigned Id = G.addNode(nullptr);
  auto *N = G.getNode(Id);
  N->Cost = Cost;
  N->IsEntry = IsEntry;
  N->IsExit = IsExit;
  return Id;
}

static void markLoopHeader(AbstractStateGraph &G, unsigned Id, unsigned Bound) {
  auto *N = G.getNode(Id);
  N->IsLoopHeader = true;
  N->UpperLoopBound = Bound;
}

// Straight-line chain: every node executes exactly once, WCET = sum of costs.
static void testChain() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, /*IsEntry=*/true);
  unsigned A = addNode(G, 3);
  unsigned B = addNode(G, 5);
  unsigned X = addNode(G, 0, /*IsEntry=*/false, /*IsExit=*/true);
  G.addEdge(E, A);
  G.addEdge(A, B);
  G.addEdge(B, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  CHECK(wcetEq(R.WCET, 8)); // 3 + 5
  // Entry executes exactly once.
  CHECK(R.ExecutionCounts.count(A) && std::llround(R.ExecutionCounts[A]) == 1);
}

// Single loop, bound N: header runs N times, body N-1 times.
//   WCET = h*N + b*(N-1)
static void testSingleLoop() {
  const unsigned H = 3, Bdy = 5, N = 10;
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, /*IsEntry=*/true);
  unsigned Hd = addNode(G, H);
  unsigned Body = addNode(G, Bdy);
  unsigned X = addNode(G, 0, false, true);
  markLoopHeader(G, Hd, N);
  G.addEdge(E, Hd);
  G.addEdge(Hd, Body);
  G.addEdge(Body, Hd, /*IsBackEdge=*/true);
  G.addEdge(Hd, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  CHECK(wcetEq(R.WCET, (long)H * N + (long)Bdy * (N - 1))); // 30 + 45 = 75
}

// Nested loops: outer bound n, inner bound m. Derived exact counts:
//   x_outerHdr = n, x_innerHdr = (n-1)*m, x_innerBody = (m-1)*(n-1),
//   x_outerLatch = n-1.
static void testNestedLoops() {
  const unsigned Co = 2, Ci = 3, Bi = 5, Cl = 7, N = 4, M = 3;
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned OH = addNode(G, Co);
  unsigned IH = addNode(G, Ci);
  unsigned IB = addNode(G, Bi);
  unsigned OL = addNode(G, Cl);
  unsigned X = addNode(G, 0, false, true);
  markLoopHeader(G, OH, N);
  markLoopHeader(G, IH, M);
  G.addEdge(E, OH);
  G.addEdge(OH, IH);
  G.addEdge(IH, IB);
  G.addEdge(IB, IH, /*IsBackEdge=*/true);
  G.addEdge(IH, OL);
  G.addEdge(OL, OH, /*IsBackEdge=*/true);
  G.addEdge(OH, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  long Expected = (long)Co * N + (long)Ci * (N - 1) * M +
                  (long)Bi * (M - 1) * (N - 1) + (long)Cl * (N - 1);
  CHECK(wcetEq(R.WCET, Expected)); // 8 + 27 + 30 + 21 = 86
}

// Diamond: the maximize objective picks the more expensive arm.
static void testDiamond() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned A = addNode(G, 10);
  unsigned B = addNode(G, 100); // expensive arm
  unsigned C = addNode(G, 1);
  unsigned D = addNode(G, 20);
  unsigned X = addNode(G, 0, false, true);
  G.addEdge(E, A);
  G.addEdge(A, B);
  G.addEdge(A, C);
  G.addEdge(B, D);
  G.addEdge(C, D);
  G.addEdge(D, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  CHECK(wcetEq(R.WCET, 130)); // 10 + 100 + 20 (B arm)
}

// Context-sensitive call/return matching (regression for commit 8e47118): the
// same callee is invoked from two sites with distinct landing blocks. Each call
// must return to its own landing, so the callee body is counted once per call
// and the inter-procedural flow stays bounded.
//   WCET = C1 + 2*FE + L1 + C2 + L2 (callee FE counted twice)
static void testTwoCallSiteMatching() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned C1 = addNode(G, 1); // first call site
  unsigned FE = addNode(G, 50);
  unsigned FR = addNode(G, 0);
  unsigned L1 = addNode(G, 2); // first landing
  unsigned C2 = addNode(G, 1); // second call site
  unsigned L2 = addNode(G, 3); // second landing
  unsigned X = addNode(G, 0, false, true);

  G.addEdge(E, C1);
  G.addEdge(C1, FE); // call edge 1
  G.addEdge(FE, FR); // callee body
  G.addEdge(FR, L1); // return edge 1
  G.addEdge(L1, C2);
  G.addEdge(C2, FE); // call edge 2
  G.addEdge(FR, L2); // return edge 2
  G.addEdge(L2, X);

  // Need a real Function* key; a unique non-null sentinel is enough because the
  // solver only uses it to group the call sites.
  const Function *F = reinterpret_cast<const Function *>(0x1);
  G.FunctionEntries[F] = FE;
  G.FunctionReturns[F] = {FR};
  G.CallSites.push_back({C1, L1, F});
  G.CallSites.push_back({C2, L2, F});

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  CHECK(wcetEq(R.WCET, 107)); // 1 + 100 + 2 + 1 + 3
  // The callee entry is executed exactly twice (once per call site).
  CHECK(R.ExecutionCounts.count(FE) &&
        std::llround(R.ExecutionCounts[FE]) == 2);
}

// Irreducible / multi-entry loop: the cycle {H,B} is entered at both H and B
// (no clean MachineLoopInfo header). The loop pass marks the closing edge B->H
// as a backedge and bounds H. The key property: the ILP stays BOUNDED -- it
// does not go unbounded the way an unmarked irreducible cycle would.
static void testIrreducibleBounded() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned A = addNode(G, 1);
  unsigned H = addNode(G, 4);
  unsigned B = addNode(G, 6);
  unsigned X = addNode(G, 0, false, true);
  markLoopHeader(G, H, 8);
  G.addEdge(E, A);
  G.addEdge(A, H); // entry into the cycle at H
  G.addEdge(A, B); // second entry into the cycle at B -> irreducible
  G.addEdge(H, B);
  G.addEdge(B, H, /*IsBackEdge=*/true); // marked backedge
  G.addEdge(H, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty()); // bounded, not unbounded
  CHECK(R.WCET > 0.0);
}

// GAP (finding #4): a loop header with UpperLoopBound == 0 gets NO loop-bound
// row, so the positive-cost cycle makes the maximize objective unbounded. Today
// the solver reports a non-empty Status (no WCET) rather than a finite bound.
static void testUnboundedLoopGap() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned Hd = addNode(G, 3);
  unsigned Body = addNode(G, 5);
  unsigned X = addNode(G, 0, false, true);
  // Mark as a loop header but leave the bound at 0 -> no constraint emitted.
  G.getNode(Hd)->IsLoopHeader = true;
  G.getNode(Hd)->UpperLoopBound = 0;
  G.addEdge(E, Hd);
  G.addEdge(Hd, Body);
  G.addEdge(Body, Hd, /*IsBackEdge=*/true);
  G.addEdge(Hd, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK_GAP(!R.Status.empty(),
            "unbounded loop (UpperLoopBound==0) yields no WCET; the ILP is "
            "unbounded -- update this test when unbounded loops are handled");
  CHECK(wcetEq(R.WCET, 0)); // no objective produced
}

// Bounded self-recursion (was finding #5's GAP). ProgramGraph::finalize bounds
// a self-recursive function by treating the callee entry as a loop header whose
// backedge is the recursive call edge, with UpperLoopBound = the supplied
// recursion_bound (max total invocations). This test mirrors that wiring and
// asserts a finite, exact WCET -- the recursion is now bounded exactly like a
// loop with bound B.
//
// Structure: f is called once from the entry (E->FE). FE (cost h) branches to
// the recursive-call block Crec (cost c) or to the base-case return FR (cost
// r); Crec->FE is the marked backedge; the return block FR flows to the
// recursive call's landing Lr (cost l) and, for the top-level invocation, to X.
// With external entry = 1 and bound B the IPET counts are: FE=B, Crec=B-1,
// FR=B, Lr=B-1, so WCET = h*B + c*(B-1) + r*B + l*(B-1).
static void testRecursionBounded() {
  const unsigned H = 10, C = 5, Rr = 2, L = 3, B = 5;
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned FE = addNode(G, H);   // F entry (recursion "header")
  unsigned Crec = addNode(G, C); // recursive call site inside F
  unsigned FR = addNode(G, Rr);  // F return block
  unsigned Lr = addNode(G, L);   // landing for the recursive call
  unsigned X = addNode(G, 0, false, true);

  markLoopHeader(G, FE, B); // recursion bound = total invocations

  G.addEdge(E, FE);
  G.addEdge(FE, Crec);                      // recurse arm
  G.addEdge(FE, FR);                        // base-case arm -> return
  G.addEdge(Crec, FE, /*IsBackEdge=*/true); // recursive call edge = backedge
  G.addEdge(FR, Lr); // return edge: F return -> recursive landing
  G.addEdge(Lr, FR); // the landed invocation also returns
  G.addEdge(FR, X);  // top-level return to exit

  const Function *F = reinterpret_cast<const Function *>(0x2);
  G.FunctionEntries[F] = FE;
  G.FunctionReturns[F] = {FR};
  G.CallSites.push_back({Crec, Lr, F});

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty()); // bounded now, not unbounded
  long Expected = (long)H * B + (long)C * (B - 1) + (long)Rr * B +
                  (long)L * (B - 1); // 50 + 20 + 10 + 12 = 92
  CHECK(wcetEq(R.WCET, Expected));
}

// Bounded mutual recursion. At the ILP level a mutual-recursion SCC is a header
// whose backedge is an inter-procedural call edge from another SCC member, so
// the solver bounds it exactly like a loop. (ProgramGraph::finalize is what
// detects the SCC, picks the header, and marks that call edge as the backedge;
// the inter-procedural graph WIRING is covered end-to-end by the cfg suite's
// mutual_recursion* tests.) Here AE models cycle-entry function a's entry
// (header, bound B) and BB the other function b's body on the cycle; b's call
// back to a (BB->AE) is the marked backedge. Counts: AE=B, BB=B-1.
static void testMutualRecursionBounded() {
  const unsigned Ca = 10, Cb = 7, B = 4;
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned AE = addNode(G, Ca);
  unsigned BB = addNode(G, Cb);
  unsigned X = addNode(G, 0, false, true);
  markLoopHeader(G, AE, B);
  G.addEdge(E, AE);
  G.addEdge(AE, BB);                // a -> b (forward)
  G.addEdge(BB, AE, /*back=*/true); // b -> a (cycle backedge)
  G.addEdge(AE, X);                 // a base case exits

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK(R.Status.empty());
  CHECK(wcetEq(R.WCET, (long)Ca * B + (long)Cb * (B - 1))); // 40 + 21 = 61
}

// Safety boundary: the SAME mutual-recursion cycle WITHOUT a header bound is
// unbounded, so the solver reports no WCET. When finalize fails to bound an SCC
// (e.g. its header has no recursion_bound) this is the formulation it produces.
static void testMutualRecursionUnboundedGap() {
  AbstractStateGraph G;
  unsigned E = addNode(G, 0, true);
  unsigned AE = addNode(G, 10);
  unsigned BB = addNode(G, 7);
  unsigned X = addNode(G, 0, false, true);
  G.addEdge(E, AE);
  G.addEdge(AE, BB);
  G.addEdge(BB, AE, /*back=*/true); // backedge present but AE not a header
  G.addEdge(AE, X);

  AbstractHighsSolver S;
  auto R = S.solveWCET(G);
  CHECK_GAP(
      !R.Status.empty(),
      "unbounded mutual-recursion cycle (no header bound) yields no WCET; "
      "update when a header-less SCC is bounded");
}

#endif // ENABLE_HIGHS

int main() {
#ifdef ENABLE_HIGHS
  testChain();
  testSingleLoop();
  testNestedLoops();
  testDiamond();
  testTwoCallSiteMatching();
  testIrreducibleBounded();
  testUnboundedLoopGap();
  testRecursionBounded();
  testMutualRecursionBounded();
  testMutualRecursionUnboundedGap();

  if (Failures == 0) {
    std::cout << "All " << Checks << " checks passed.\n";
    return 0;
  }
  std::cerr << Failures << " of " << Checks << " checks FAILED.\n";
  return 1;
#else
  std::cout << "ILP solver tests skipped: no ILP backend enabled "
               "(reconfigure with HiGHS).\n";
  return 0;
#endif
}
