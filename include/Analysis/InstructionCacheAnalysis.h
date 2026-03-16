#ifndef INSTRUCTION_CACHE_ANALYSIS_H
#define INSTRUCTION_CACHE_ANALYSIS_H

// =============================================================================
// InstructionCacheAnalysis — Template for Abstract Cache Analyses
// =============================================================================
//
// PURPOSE
//   This file is the canonical template for implementing a new abstract
//   analysis in LLTA. It implements a *dummy* instruction cache analysis that
//   models an always-hit cache (zero miss penalty) and can be plugged directly
//   into any PipelineAnalysis subclass (e.g. MSP430Pipeline).
//
//   Copy this file, rename the classes, and fill in the TODO sections to
//   implement a real cache analysis (must/may, age-based, persistence, etc.).
//
// FRAMEWORK OVERVIEW
//   Every abstract analysis in LLTA consists of two cooperating classes:
//
//     AbstractState subclass      — describes what is known at one program point
//     AbstractAnalysable subclass — the transfer function that updates the state
//
//   The WorklistSolver drives the fixpoint computation:
//     for each basic block:
//       InState = join(predecessor states)     -- meet operator
//       for each instruction MI in the block:
//         Cost += analysis.process(InState, MI) -- transfer function
//       if InState changed: re-add successors to worklist
//
//   PipelineAnalysis is a composite that holds N sub-analyses. Its
//   PipelineState contains one sub-state per sub-analysis. Costs are summed
//   across all sub-analyses per instruction (see PipelineAnalysis::process).
//   To add a new analysis to a target pipeline, call addAnalysis() in the
//   target's PipelineAnalysis constructor (e.g. MSP430Pipeline).
//
// HOW TO ADD A REAL CACHE ANALYSIS
//   1. Replace AccessCount with your abstract cache domain (e.g. an age matrix,
//      a set of abstract cache lines, must/may sets).
//   2. In join(): implement the correct meet operator for your domain
//      (e.g. set union for may-analysis, set intersection for must-analysis).
//   3. In process(): implement the transfer function:
//      a. Determine which cache set(s) are accessed by MI (from operands /
//         address analysis results stored in TimingAnalysisResults).
//      b. Update the abstract cache state (evict/insert cache lines).
//      c. Return 0 if a guaranteed hit, CacheMissPenalty if a guaranteed or
//         possible miss (safe WCET over-approximation).
//   4. Adjust CacheMissPenalty in the target pipeline constructor.
//
// =============================================================================

#include "Analysis/AbstractAnalysable.h"
#include "Analysis/AbstractState.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <memory>
#include <string>

namespace llvm {

// =============================================================================
// InstructionCacheState
// =============================================================================
//
// AbstractState subclass for instruction cache analysis.
//
// In a real implementation this class would hold the abstract description of
// the cache contents at a program point — for example:
//
//   Must-cache analysis  : the set of cache lines *guaranteed* to be cached.
//   May-cache analysis   : the set of cache lines that *might* be cached.
//   Age-based analysis   : for each cache line, an upper bound on its age
//                          (position in the LRU stack); lines with age >=
//                          associativity are classified as "may miss".
//
// For this dummy implementation the state tracks only the number of
// instruction accesses seen on the current path (AccessCount). This is
// sufficient to exercise the join/equals/clone logic without a real model.
//
// TEMPLATE NOTES
//   equals() must return true iff the two states represent identical abstract
//   information. The WorklistSolver uses it to detect fixpoint.
//
//   join() must be monotone and over-approximate the set of concrete states
//   reachable from either predecessor. It must return true iff the state
//   actually changed (to avoid infinite worklist re-insertions).
//
class InstructionCacheState : public AbstractState {
public:
  // -------------------------------------------------------------------------
  // State representation
  // -------------------------------------------------------------------------
  // Dummy placeholder: counts instruction accesses on the current path.
  //
  // TODO: Replace with your cache model, for example:
  //   unsigned NumSets;
  //   unsigned Associativity;
  //   std::vector<std::vector<uint64_t>> AgeMatrix; // [set][age] = line tag
  //
  unsigned AccessCount;

  explicit InstructionCacheState(unsigned Count = 0) : AccessCount(Count) {}

  // -------------------------------------------------------------------------
  // clone() — deep copy
  // -------------------------------------------------------------------------
  // Must return an independent copy. The WorklistSolver clones predecessor
  // states before joining, so mutations to the clone must not affect the
  // original.
  //
  std::unique_ptr<AbstractState> clone() const override {
    return std::make_unique<InstructionCacheState>(AccessCount);
  }

  // -------------------------------------------------------------------------
  // equals() — fixpoint check
  // -------------------------------------------------------------------------
  // Returns true iff *this and *Other represent identical abstract states.
  // Called by the WorklistSolver after processing a node to determine whether
  // successors need to be re-enqueued.
  //
  // TODO: Compare all fields of your real cache model, not just AccessCount.
  //
  bool equals(const AbstractState *Other) const override {
    const auto *O = static_cast<const InstructionCacheState *>(Other);
    return AccessCount == O->AccessCount;
  }

  // -------------------------------------------------------------------------
  // join() — meet operator at control-flow join points
  // -------------------------------------------------------------------------
  // Merges *Other into *this. Must be:
  //   - sound:    the result over-approximates both input states
  //   - monotone: the result is always >= both inputs in the lattice order
  //
  // Returns true iff *this was modified (i.e. information was lost / the
  // state widened). Returning false when nothing changed is an optimisation
  // that prevents the WorklistSolver from unnecessarily re-processing nodes.
  //
  // Dummy: take the maximum AccessCount (pessimistic upper bound).
  //
  // TODO: For a must-cache analysis use intersection (remove lines not in
  //       both predecessors). For a may-cache analysis use union.
  //
  bool join(const AbstractState *Other) override {
    const auto *O = static_cast<const InstructionCacheState *>(Other);
    if (O->AccessCount > AccessCount) {
      AccessCount = O->AccessCount;
      return true; // state changed
    }
    return false; // state unchanged — fixpoint holds for this input
  }

  // -------------------------------------------------------------------------
  // toString() — debug / dot-graph output
  // -------------------------------------------------------------------------
  // Used by AbstractStateGraph when dumping .dot files for inspection.
  // Keep it concise but informative.
  //
  std::string toString() const override {
    return "ICache(accesses=" + std::to_string(AccessCount) + ")";
  }
};

// =============================================================================
// InstructionCacheAnalysis
// =============================================================================
//
// AbstractAnalysable subclass — implements the transfer function for
// instruction cache analysis.
//
// The transfer function (process) is the core of the analysis. It is called
// once per MachineInstr and must:
//   1. Update the abstract cache state to reflect that MI was fetched.
//   2. Return the *additional* cycle cost incurred (0 = hit, N = miss).
//
// This dummy always returns 0 (all fetches hit the cache) so it does not
// affect the WCET result. Replace the body of process() with a real model.
//
// INTEGRATION
//   Instantiate this class and register it with a PipelineAnalysis via
//   addAnalysis(). Example (in MSP430Pipeline constructor):
//
//     addAnalysis(std::make_unique<InstructionCacheAnalysis>(/*penalty=*/0));
//
//   For a real target with a 3-cycle miss penalty pass 3 instead of 0.
//   The PipelineAnalysis::process() sums the costs from all sub-analyses
//   (execution stage + cache stage + ...) per instruction.
//
class InstructionCacheAnalysis : public AbstractAnalysable {
public:
  // -------------------------------------------------------------------------
  // Constructor
  // -------------------------------------------------------------------------
  // MissPenalty: extra cycles added to the WCET estimate when a cache miss
  // is detected (or conservatively assumed). Set to 0 for the dummy.
  //
  // TODO: Set to the target's actual instruction-fetch miss penalty, e.g.:
  //   MSP430  : not applicable (no instruction cache — keep 0)
  //   ESP32-C6: ~5 cycles for a cache miss on the first access to a line
  //
  explicit InstructionCacheAnalysis(unsigned MissPenalty = 0)
      : CacheMissPenalty(MissPenalty) {}

  // -------------------------------------------------------------------------
  // getInitialState() — bottom / empty-cache state
  // -------------------------------------------------------------------------
  // Returns the state at the program entry: the cache is cold (empty).
  // For a must-cache analysis the initial must-set is empty.
  // For a may-cache analysis the initial may-set is also empty.
  //
  std::unique_ptr<AbstractState> getInitialState() override {
    // TODO: Initialise your real cache model here (e.g. empty age matrix).
    return std::make_unique<InstructionCacheState>(/*AccessCount=*/0);
  }

  // -------------------------------------------------------------------------
  // process() — transfer function
  // -------------------------------------------------------------------------
  // Called once per MachineInstr MI during the worklist fixpoint computation.
  // Modifies *State in-place to reflect the cache effect of fetching MI.
  // Returns the additional cycle cost (0 for a hit, CacheMissPenalty for a
  // miss or conservatively assumed miss).
  //
  // TODO: Replace the dummy body with a real model:
  //
  //   Step 1 — Determine the cache address of MI.
  //     The instruction's PC can be approximated from the symbol address
  //     resolved by AdressResolverPass (stored in TimingAnalysisResults).
  //     Compute: CacheSet = (PC / LineSize) % NumSets
  //              LineTag  = PC / (LineSize * NumSets)
  //
  //   Step 2 — Look up the line in the abstract cache state.
  //     Must-analysis: Is this line guaranteed present?
  //       If age[set][line] < Associativity -> guaranteed hit -> return 0
  //       Otherwise                          -> guaranteed miss -> age-update
  //     May-analysis: Could this line be absent?
  //       If line not in may-set -> guaranteed miss -> return CacheMissPenalty
  //       Otherwise              -> may-hit (return CacheMissPenalty for WCET)
  //
  //   Step 3 — Update the abstract cache state.
  //     Insert the accessed line; increment ages of other lines in the same
  //     set (LRU model); evict lines whose age reaches Associativity.
  //
  //   Step 4 — Return the cycle cost.
  //     0 for a guaranteed hit; CacheMissPenalty otherwise.
  //
  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    auto *CState = static_cast<InstructionCacheState *>(State);

    // Dummy: count the access and assume a hit (zero extra cycles).
    CState->AccessCount++;

    // TODO: Replace with the real analysis described above.
    (void)MI; // suppress unused-parameter warning until implemented

    // Dummy: always a hit — contributes 0 extra cycles to WCET.
    // TODO: Replace with: return CacheMissPenalty; for a conservative
    //       always-miss model, or with the real hit/miss decision logic.
    return 0 * CacheMissPenalty;
  }

private:
  // Cycles added to the WCET estimate per instruction cache miss.
  // Passed at construction time so the same analysis class can be reused
  // with different penalties for different target configurations.
  unsigned CacheMissPenalty;
};

} // namespace llvm

#endif // INSTRUCTION_CACHE_ANALYSIS_H
