//===- ProgramGraphTests.cpp - unit tests for ProgramGraph construction ---===//
//
// A dependency-light standalone test binary (no GoogleTest) exercising the
// hand-constructable surface of the legacy ProgramGraph (lib/Graph/): node and
// edge insertion, successor/predecessor symmetry, edge queries, edge removal,
// the removeNode() isFree() invariant, and the BackEdgePredecessors bookkeeping
// the IPET loop-bound row keys off of.
//
// The MachineFunction-driven paths -- fillGraphWithFunction(), finalize()'s call
// wiring / reachability pruning / irreducible-backedge marking -- need a live
// MachineFunction and MachineModuleInfo, so they are covered end-to-end by the
// tests/cfg/*.ll integration suite instead. These unit tests pin the graph data
// structure those passes build on.
//
// Run via CTest (`ctest -R LLTAProgramGraphTests`) or `check-llta-cfg`.
//===----------------------------------------------------------------------===//

#include "Graph/ProgramGraph.h"

#include <iostream>
#include <memory>

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

static unsigned addNode(ProgramGraph &G, unsigned Cycles) {
  return G.addNode(std::make_unique<MuArchState>(Cycles, Cycles), nullptr);
}

// Node/edge insertion and successor/predecessor symmetry.
static void testNodesAndEdges() {
  ProgramGraph G;
  unsigned A = addNode(G, 1);
  unsigned B = addNode(G, 2);
  unsigned C = addNode(G, 3);
  G.addEdge(A, B);
  G.addEdge(A, C);

  CHECK(G.getNodes().size() == 3);
  CHECK(G.hasEdge(A, B));
  CHECK(G.hasEdge(A, C));
  CHECK(!G.hasEdge(B, A));

  // Successors of A are {B, C}; B and C each have A as a predecessor.
  CHECK(G.getSuccessors(A).size() == 2);
  CHECK(G.getSuccessors(A).count(B) == 1);
  CHECK(G.getSuccessors(A).count(C) == 1);
  CHECK(G.getPredecessors(B).count(A) == 1);
  CHECK(G.getPredecessors(C).count(A) == 1);

  // Node carries its assigned latency.
  CHECK(G.getNodes().at(B).getState().getUpperBoundCycles() == 2);
}

// removeEdge detaches both endpoints; removeNode requires the node to be free.
static void testRemoval() {
  ProgramGraph G;
  unsigned A = addNode(G, 1);
  unsigned B = addNode(G, 2);
  G.addEdge(A, B);
  CHECK(!G.isFree(A)); // has an outgoing edge
  CHECK(!G.isFree(B)); // has an incoming edge

  G.removeEdge(A, B);
  CHECK(!G.hasEdge(A, B));
  CHECK(G.isFree(A));
  CHECK(G.isFree(B));

  // A free node can be removed (removeNode asserts isFree()).
  G.removeNode(B);
  CHECK(G.getNodes().size() == 1);
  CHECK(G.getNodes().count(A) == 1);
}

// Self-loop: a node may be its own successor and predecessor.
static void testSelfLoop() {
  ProgramGraph G;
  unsigned A = addNode(G, 1);
  G.addEdge(A, A);
  CHECK(G.hasEdge(A, A));
  CHECK(G.getSuccessors(A).count(A) == 1);
  CHECK(G.getPredecessors(A).count(A) == 1);
}

// BackEdgePredecessors is the set the loop-bound row reads to find the backedge
// flowing into a header. Marking it is independent of the forward edge set.
static void testBackEdgeBookkeeping() {
  ProgramGraph G;
  unsigned Hdr = addNode(G, 1);
  unsigned Latch = addNode(G, 2);
  G.addEdge(Hdr, Latch);
  G.addEdge(Latch, Hdr); // the loop-closing edge

  // Mark the latch as a backedge predecessor of the header (what
  // fillGraphWithFunction does from MachineLoopInfo / IrreducibleBackEdges).
  G.Nodes.at(Hdr).BackEdgePredecessors.insert(Latch);
  CHECK(G.Nodes.at(Hdr).BackEdgePredecessors.count(Latch) == 1);
  // The forward edge from header to latch is not a backedge.
  CHECK(G.Nodes.at(Latch).BackEdgePredecessors.count(Hdr) == 0);
}

// wireEntryExit() is the pure helper that connects an entry function's synthetic
// Entry/Exit nodes. These cases exercise the two shapes that are NOT reachable
// through the C->ELF toolchain (they need a live MachineFunction), so the helper
// is the only place they can be tested.

// Empty function (zero body blocks): must wire Entry -> Exit directly and never
// read an uninitialized node id (the former bug).
static void testWireEntryExitEmpty() {
  ProgramGraph G;
  unsigned Entry = addNode(G, 0);
  unsigned Exit = addNode(G, 0);
  bool Ok = G.wireEntryExit(/*BodyNodeIds=*/{}, /*ReturnNodeIds=*/{}, Entry, Exit);
  CHECK(Ok);
  CHECK(G.hasEdge(Entry, Exit));
}

// Function with blocks but no return block (noreturn / infinite loop): fall back
// to wiring the last block in layout order to Exit.
static void testWireEntryExitNoReturn() {
  ProgramGraph G;
  unsigned Entry = addNode(G, 0);
  unsigned Exit = addNode(G, 0);
  unsigned B0 = addNode(G, 1);
  unsigned B1 = addNode(G, 2);
  bool Ok = G.wireEntryExit(/*BodyNodeIds=*/{B0, B1}, /*ReturnNodeIds=*/{}, Entry, Exit);
  CHECK(Ok);
  CHECK(G.hasEdge(Entry, B0));      // entry -> first block
  CHECK(G.hasEdge(B1, Exit));       // fallback: last block -> exit
  CHECK(!G.hasEdge(Entry, Exit));   // not the empty-function shortcut
}

// Normal function with multiple return blocks: entry -> first, every return -> exit.
static void testWireEntryExitMultipleReturns() {
  ProgramGraph G;
  unsigned Entry = addNode(G, 0);
  unsigned Exit = addNode(G, 0);
  unsigned B0 = addNode(G, 1);
  unsigned R1 = addNode(G, 2);
  unsigned R2 = addNode(G, 3);
  bool Ok = G.wireEntryExit(/*BodyNodeIds=*/{B0, R1, R2}, /*ReturnNodeIds=*/{R1, R2},
                            Entry, Exit);
  CHECK(Ok);
  CHECK(G.hasEdge(Entry, B0));
  CHECK(G.hasEdge(R1, Exit));
  CHECK(G.hasEdge(R2, Exit));
  CHECK(!G.hasEdge(B0, Exit)); // non-return block is not wired to exit
}

int main() {
  testNodesAndEdges();
  testRemoval();
  testSelfLoop();
  testBackEdgeBookkeeping();
  testWireEntryExitEmpty();
  testWireEntryExitNoReturn();
  testWireEntryExitMultipleReturns();

  if (Failures == 0) {
    std::cout << "All " << Checks << " checks passed.\n";
    return 0;
  }
  std::cerr << Failures << " of " << Checks << " checks FAILED.\n";
  return 1;
}
