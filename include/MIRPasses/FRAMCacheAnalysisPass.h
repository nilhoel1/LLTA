#ifndef LLVM_LLTA_MIRPASSES_FRAMCACHEANALYSISPASS_H
#define LLVM_LLTA_MIRPASSES_FRAMCACHEANALYSISPASS_H

#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

/**
 * FRAMCacheAnalysisPass — MSP430 FRAM read-cache must-analysis.
 *
 * A cache-aware refinement of FRAMWaitStatePass. Instead of charging every FRAM
 * fetch word the wait-state penalty, it runs a sound must-analysis of the FR5xx
 * read cache and charges the penalty only for fetch words that are not provably
 * cache hits. The dominant win is spatial locality: in straight-line code the
 * first word of each 8-byte line misses and the remaining words hit.
 *
 * It is built entirely from the modular cache components in include/Analysis/
 * Cache/ (CacheGeometry + ReplacementPolicy + CacheAccessMapper, driven by the
 * generic CacheAnalysis engine) and executed through the existing abstract-
 * interpretation framework's WorklistSolver, which performs the cross-block CFG
 * fixpoint. The per-block must-analysis penalty is added into MBBLatencyMap, so
 * it flows into the WCET exactly like the FRAMWaitStatePass penalty it replaces.
 *
 * Under -fram-cache-verbose it also runs a sound may-analysis and reports
 * accesses proven never cached ("always-miss"); that is diagnostic only and
 * does not change the WCET.
 *
 * Must run after InstructionLatencyPass (which populates MBBLatencyMap) and
 * after AdressResolverPass (addresses + FRAMStart). No-op unless -fram-cache is
 * set, -fram-wait-states > 0, and -fram-start was supplied. When enabled it
 * supersedes FRAMWaitStatePass (which then skips itself to avoid double-count).
 */
class FRAMCacheAnalysisPass : public MachineFunctionPass {
public:
  static char ID;
  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;

  FRAMCacheAnalysisPass(TimingAnalysisResults &TAR);

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  bool runOnMachineFunction(MachineFunction &F) override;

  virtual llvm::StringRef getPassName() const override {
    return "MSP430 FRAM Cache Must-Analysis Pass";
  }
};

MachineFunctionPass *createFRAMCacheAnalysisPass(TimingAnalysisResults &TAR);
} // namespace llvm

#endif // LLVM_LLTA_MIRPASSES_FRAMCACHEANALYSISPASS_H
