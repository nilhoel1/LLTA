#include "Targets/MSP430/FRAMCacheAnalysisPass.h"
#include "Analysis/AbstractStateGraph.h"
#include "Analysis/Cache/CacheAnalysis.h"
#include "Analysis/Cache/CacheGeometry.h"
#include "Analysis/Cache/FRAMAccessMapper.h"
#include "Analysis/Cache/ReplacementPolicy.h"
#include "Analysis/WorklistSolver.h"
#include "Targets/MSP430/MSP430Options.h"
#include "TimingAnalysisResults.h"
#include "Utility/InstructionWords.h"
#include "Utility/Options.h"

#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace llvm {

char FRAMCacheAnalysisPass::ID = 0;

FRAMCacheAnalysisPass::FRAMCacheAnalysisPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

/// Build the must-analysis replacement policy selected by -fram-cache-policy.
static std::unique_ptr<ReplacementPolicy> makeMustPolicy(const CacheGeometry &Geo) {
  if (FRAMCachePolicy == "lru")
    return std::make_unique<LRUPolicy>(Geo.Ways);
  if (FRAMCachePolicy == "fifo")
    return std::make_unique<FIFOPolicy>(Geo.Ways);
  return std::make_unique<UnknownPolicy>(); // default / "unknown"
}

/// Emit "always-miss" diagnostics: run a sound may-analysis (LRU-may with the
/// real associativity, which over-approximates the may-set of ANY replacement
/// policy, so the reports are sound even on the undocumented FR5994) and, after
/// the fixpoint, replay each block from its converged entry state to report the
/// accesses proven never cached.
static void reportAlwaysMiss(MachineFunction &F, const TimingAnalysisResults &TAR,
                             CacheGeometry Geo,
                             const std::unordered_map<const MachineInstr *,
                                                      unsigned> &Words) {
  LRUPolicy MayPolicy(Geo.Ways);
  FRAMAccessMapper Mapper(TAR, Geo, Words);
  CacheAnalysis May(Geo, FRAMWaitStates, MayPolicy, Mapper, AnalysisKind::May);

  AbstractStateGraph ASG;
  WorklistSolver Solver(May, ASG);
  Solver.run(F, /*MLI=*/nullptr, /*LoopBounds=*/nullptr); // sink off ⇒ no output

  // Map each node to its MBB for predecessor lookups during replay.
  // Replay from the converged entry state so the classification is final.
  std::set<std::string> Reported; // dedupe "block@line"
  May.setDefiniteMissSink([&](const MachineInstr *MI, uint64_t LineId) {
    std::string Key = std::string(MI->getParent()->getName()) + "@" +
                      Twine::utohexstr(LineId).str();
    if (Reported.insert(Key).second)
      outs() << "[fram-cache] always-miss: line 0x" << Twine::utohexstr(LineId)
             << " in " << F.getName() << ":" << MI->getParent()->getName()
             << "\n";
  });

  for (const auto &Pair : ASG.getNodes()) {
    const AbstractStateGraph::Node *N = Pair.second.get();
    if (!N->MBB)
      continue;
    // Entry state = join of predecessors' converged (out) states; cold if none.
    std::unique_ptr<AbstractState> In;
    for (unsigned PredId : ASG.getPredecessors(N->Id)) {
      const auto *Pred = ASG.getNode(PredId);
      if (!Pred)
        continue;
      if (!In)
        In = Pred->State->clone();
      else
        In->join(Pred->State.get());
    }
    if (!In)
      In = May.getInitialState();
    for (const MachineInstr &MI : *N->MBB)
      May.process(In.get(), &MI);
  }
}

bool FRAMCacheAnalysisPass::runOnMachineFunction(MachineFunction &F) {
  // No-op unless the cache model is enabled and configured. This keeps default
  // runs (and the regression tests) unchanged; FRAMWaitStatePass handles the
  // no-cache case.
  if (!FRAMCache || FRAMWaitStates == 0 || !TAR.hasFRAMStart())
    return false;

  CacheGeometry Geo(FRAMCacheSets, FRAMCacheWays, FRAMCacheLineBytes);
  if (!Geo.isValid()) {
    errs() << "[fram-cache] warning: invalid geometry (sets=" << FRAMCacheSets
           << ", ways=" << FRAMCacheWays << ", line-bytes=" << FRAMCacheLineBytes
           << "); sets and line-bytes must be powers of two. Skipping.\n";
    return false;
  }

  // Per-instruction fetch word counts (shared with FRAMWaitStatePass).
  std::unordered_map<const MachineInstr *, unsigned> Words =
      computeInstructionWords(F, TAR);

  // --- Must-analysis: the cache-aware fetch penalty fed into the WCET. ---
  // Assemble the generic engine from the modular parts. Order of declaration
  // matters: Policy/Mapper/Words must outlive the analysis that references them.
  std::unique_ptr<ReplacementPolicy> Policy = makeMustPolicy(Geo);
  FRAMAccessMapper Mapper(TAR, Geo, Words);
  CacheAnalysis Must(Geo, FRAMWaitStates, *Policy, Mapper, AnalysisKind::Must);

  // Run the cross-block fixpoint through the abstract-interpretation framework.
  AbstractStateGraph ASG;
  WorklistSolver Solver(Must, ASG);
  Solver.run(F, /*MLI=*/nullptr, /*LoopBounds=*/nullptr);

  // Fold the per-block cache penalty into the latency path.
  auto Map = TAR.getMBBLatencyMap();
  unsigned FuncPenalty = 0;
  for (const auto &Pair : ASG.getNodes()) {
    const AbstractStateGraph::Node *N = Pair.second.get();
    if (N->MBB && N->Cost) {
      Map[N->MBB] += N->Cost;
      FuncPenalty += N->Cost;
    }
  }
  TAR.setMBBLatencyMap(Map);

  if (DebugPrints || AddressResolverVerbose || FRAMCacheVerbose)
    outs() << "[fram-cache] " << F.getName() << ": +" << FuncPenalty
           << " cycle(s) (policy=" << Policy->name() << ", " << Geo.NumSets
           << " set(s) x " << Geo.Ways << " way(s), " << Geo.LineBytes
           << "B lines, " << FRAMWaitStates << " wait state(s)/miss)\n";

  // --- May-analysis: always-miss diagnostics (no WCET impact). ---
  if (FRAMCacheVerbose)
    reportAlwaysMiss(F, TAR, Geo, Words);

  return false;
}

MachineFunctionPass *createFRAMCacheAnalysisPass(TimingAnalysisResults &TAR) {
  return new FRAMCacheAnalysisPass(TAR);
}

} // namespace llvm
