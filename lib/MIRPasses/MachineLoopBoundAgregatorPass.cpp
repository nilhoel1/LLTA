#include "MIRPasses/MachineLoopBoundAgregatorPass.h"
#include "Targets/RTTarget.h"
#include "Utility/Options.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define DEBUG_TYPE "machine-loop-bound-aggregator"

namespace llvm {

// Helper structure to store loop bound data from JSON
namespace {
struct JSONLoopBound {
  std::string FileName;
  unsigned Line;
  unsigned Column;
  unsigned LowerBound;
  unsigned UpperBound;
};
} // namespace

// Helper function to load loop bounds from JSON file
static std::vector<JSONLoopBound>
loadLoopBoundsFromJSON(const std::string &JSONPath) {
  std::vector<JSONLoopBound> Bounds;

  if (JSONPath.empty()) {
    return Bounds;
  }

  auto BufferOrErr = MemoryBuffer::getFile(JSONPath);
  if (!BufferOrErr) {
    outs() << "Warning: Could not open loop bounds JSON file: " << JSONPath
           << "\n";
    return Bounds;
  }

  auto JSONOrErr = json::parse(BufferOrErr.get()->getBuffer());
  if (!JSONOrErr) {
    outs() << "Warning: Could not parse loop bounds JSON file: " << JSONPath
           << "\n";
    return Bounds;
  }

  json::Object *Root = JSONOrErr->getAsObject();
  if (!Root) {
    outs() << "Warning: JSON root is not an object\n";
    return Bounds;
  }

  json::Array *LoopBoundsArray = Root->getArray("loop_bounds");
  if (!LoopBoundsArray) {
    outs() << "Warning: No 'loop_bounds' array found in JSON\n";
    return Bounds;
  }

  for (const auto &Item : *LoopBoundsArray) {
    const json::Object *LoopObj = Item.getAsObject();
    if (!LoopObj)
      continue;

    JSONLoopBound Bound;
    if (auto File = LoopObj->getString("file"))
      Bound.FileName = File->str();
    if (auto Line = LoopObj->getInteger("line"))
      Bound.Line = *Line;
    if (auto Col = LoopObj->getInteger("column"))
      Bound.Column = *Col;
    if (auto Lower = LoopObj->getInteger("lower_bound"))
      Bound.LowerBound = *Lower;
    if (auto Upper = LoopObj->getInteger("upper_bound"))
      Bound.UpperBound = *Upper;

    Bounds.push_back(Bound);
  }

  outs() << "Loaded " << Bounds.size()
         << " loop bounds from JSON file: " << JSONPath << "\n";
  return Bounds;
}

// Load recursion bounds (function name -> max total invocations) from the same
// JSON file, under a "recursion_bounds" array of {"function", "bound"} objects.
// Returns an empty map if the file or array is absent (recursion bounds are
// optional; only recursive functions need them).
static std::map<std::string, unsigned>
loadRecursionBoundsFromJSON(const std::string &JSONPath) {
  std::map<std::string, unsigned> Bounds;
  if (JSONPath.empty())
    return Bounds;

  auto BufferOrErr = MemoryBuffer::getFile(JSONPath);
  if (!BufferOrErr)
    return Bounds;
  auto JSONOrErr = json::parse(BufferOrErr.get()->getBuffer());
  if (!JSONOrErr)
    return Bounds;
  json::Object *Root = JSONOrErr->getAsObject();
  if (!Root)
    return Bounds;
  json::Array *Arr = Root->getArray("recursion_bounds");
  if (!Arr)
    return Bounds;

  for (const auto &Item : *Arr) {
    const json::Object *Obj = Item.getAsObject();
    if (!Obj)
      continue;
    auto Func = Obj->getString("function");
    auto Bound = Obj->getInteger("bound");
    if (Func && Bound && *Bound > 0)
      Bounds[Func->str()] = static_cast<unsigned>(*Bound);
  }
  if (!Bounds.empty())
    outs() << "Loaded " << Bounds.size()
           << " recursion bounds from JSON file: " << JSONPath << "\n";
  return Bounds;
}

// Iterative DFS over the machine CFG rooted at the entry block. Returns the
// back edges (edges to a block currently on the DFS stack) -- the loop-closing
// edges. Unlike MachineLoopInfo this also finds the backedges of *irreducible*
// (multi-entry) loops, e.g. a switch jump-table forming Duff's device.
static std::set<std::pair<const MachineBasicBlock *, const MachineBasicBlock *>>
findBackEdges(MachineFunction &MF) {
  std::set<std::pair<const MachineBasicBlock *, const MachineBasicBlock *>>
      Back;
  if (MF.empty())
    return Back;
  enum Color { White, Gray, Black };
  std::map<const MachineBasicBlock *, Color> Colors;
  std::vector<std::pair<MachineBasicBlock *, MachineBasicBlock::succ_iterator>>
      Stack;
  MachineBasicBlock *Entry = &MF.front();
  Colors[Entry] = Gray;
  Stack.push_back({Entry, Entry->succ_begin()});
  while (!Stack.empty()) {
    MachineBasicBlock *BB = Stack.back().first;
    if (Stack.back().second == BB->succ_end()) {
      Colors[BB] = Black;
      Stack.pop_back();
      continue;
    }
    MachineBasicBlock *Succ = *Stack.back().second;
    ++Stack.back().second; // advance before any push (which may reallocate)
    Color &C = Colors[Succ];
    if (C == White) {
      C = Gray;
      Stack.push_back({Succ, Succ->succ_begin()});
    } else if (C == Gray) {
      Back.insert({BB, Succ}); // Succ is on the stack -> back edge
    }
  }
  return Back;
}

// The blocks of the loop whose header is \p H: those reachable from H that can
// also reach H (the strongly-connected region around the header). Used to match
// a JSON pragma to an irreducible loop MachineLoopInfo did not recognize.
static std::set<const MachineBasicBlock *>
computeLoopBody(MachineBasicBlock *H) {
  std::set<const MachineBasicBlock *> Fwd, Bwd;
  std::vector<MachineBasicBlock *> WL{H};
  while (!WL.empty()) {
    MachineBasicBlock *B = WL.back();
    WL.pop_back();
    for (MachineBasicBlock *S : B->successors())
      if (Fwd.insert(S).second)
        WL.push_back(S);
  }
  WL = {H};
  while (!WL.empty()) {
    MachineBasicBlock *B = WL.back();
    WL.pop_back();
    for (MachineBasicBlock *P : B->predecessors())
      if (Bwd.insert(P).second)
        WL.push_back(P);
  }
  std::set<const MachineBasicBlock *> Body{H};
  for (const MachineBasicBlock *B : Fwd)
    if (Bwd.count(B))
      Body.insert(B);
  return Body;
}

char MachineLoopBoundAgregatorPass::ID = 0;

MachineLoopBoundAgregatorPass::MachineLoopBoundAgregatorPass(
    TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

void MachineLoopBoundAgregatorPass::getAnalysisUsage(AnalysisUsage &AU) const {
  MachineFunctionPass::getAnalysisUsage(AU);
  AU.setPreservesAll();
  AU.addRequired<MachineLoopInfoWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
}

bool MachineLoopBoundAgregatorPass::runOnMachineFunction(MachineFunction &F) {
  MachineLoopInfo &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();

  std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBounds;

  // Load loop bounds from JSON file if specified
  std::vector<JSONLoopBound> JSONBounds =
      loadLoopBoundsFromJSON(LoopBoundsJSON);
  std::unordered_map<std::string, unsigned> JSONBoundsByLocation;

  // Recursion bounds are function-keyed and module-wide, not per-loop. Load
  // once and publish to the shared results so ProgramGraph::finalize can bound
  // self-recursive call edges. (This pass runs per function; re-publishing the
  // same map each time is idempotent.)
  TAR.setRecursionBoundMap(loadRecursionBoundsFromJSON(LoopBoundsJSON));

  // Create a lookup map by location (filename:line) - ignore column as it may
  // vary
  for (const auto &Bound : JSONBounds) {
    // Key by basename so it matches the basename-based lookup below: the JSON
    // "file" field holds a full source path while DebugLoc filenames may not.
    StringRef Base = sys::path::filename(Bound.FileName);
    std::string Key = (Base + ":" + std::to_string(Bound.Line)).str();
    // Use the upper bound as the trip count
    JSONBoundsByLocation[Key] = Bound.UpperBound;
    if (DebugPrints)
      outs() << "  JSON bound at " << Key << ": " << Bound.UpperBound << "\n";
  }
  if (DebugPrints) {
    outs() << "Processing function: " << F.getName() << "\n";
    outs() << "  Machine loops found: " << MLI.getLoopsInPreorder().size()
           << "\n";
    outs() << "  IR loops found: " << LI.getLoopsInPreorder().size() << "\n";
  }

  // Iterate over all loops in preorder
  for (auto *ML : MLI.getLoopsInPreorder()) {
    MachineBasicBlock *Header = ML->getHeader();
    const BasicBlock *BB = Header->getBasicBlock();

    if (DebugPrints)
      outs() << "  Machine loop with header MBB " << Header->getNumber() << " ("
             << Header->getName() << ")";

    // Try to recover the matching IR loop. After loop-rotation + ISel a machine
    // loop's header does not always map cleanly to the IR loop header: its IR
    // block can be a rotated guard/pre-header (so LoopInfo reports a different
    // header, or none), or the machine block may carry no IR block at all. A
    // clean IR loop unlocks SCEV trip counts and !llvm.loop start-location
    // debug info; when it is missing we do NOT drop the loop (that would leave
    // it unbounded) -- we fall back to the machine-block pragma scan below.
    Loop *L = BB ? LI.getLoopFor(BB) : nullptr;
    bool CleanIRLoop = L && L->getHeader() == BB;

    if (DebugPrints) {
      if (!BB)
        outs() << " - no IR BasicBlock mapped";
      else
        outs() << " maps to IR BB " << BB->getName();
      if (BB && !L)
        outs() << " - no IR loop for this BB";
      else if (L && !CleanIRLoop)
        outs() << " - IR loop header mismatch (" << L->getHeader()->getName()
               << ")";
      else if (CleanIRLoop)
        outs() << " - found matching IR loop";
      outs() << "\n";
    }

    unsigned TripCount = 0;
    // The loose SCEV max-backedge-taken count (pessimistic: the index type's
    // maximum for loops SCEV cannot bound) is computed in the clean-IR path
    // below but applied only as the very last resort -- after BOTH the
    // start-location JSON lookup and the machine-block JSON scan. Declared here
    // so it survives the CleanIRLoop scope.
    unsigned MaxBTCTripCount = 0;

    if (CleanIRLoop) {
      // First try to get trip count from SCEV.
      TripCount = SE.getSmallConstantTripCount(L);
      // getSmallConstantTripCount returns 0 if unknown or not constant.
      // It also returns the exact trip count, not the bound.
      // But for timing analysis, exact trip count is often what we want if it's
      // constant. If it's not constant, we might want max backedge taken count.
      if (DebugPrints)
        outs() << "    - SmallConstantTripCount: " << TripCount << "\n";

      // SCEV's constant max-backedge-taken count is only a *pessimistic*
      // over-approximation: for a loop SCEV cannot bound, it returns the index
      // type's maximum (e.g. 32766 on a 16-bit target). Compute it here but do
      // NOT let it shadow a tight JSON pragma bound -- it is applied only as a
      // last resort, after the JSON lookup below.
      if (TripCount == 0) {
        const SCEV *MaxBTC = SE.getConstantMaxBackedgeTakenCount(L);
        if (DebugPrints)
          outs() << "    - Trying max backedge taken count: " << *MaxBTC
                 << "\n";
        if (auto *C = dyn_cast<SCEVConstant>(MaxBTC)) {
          MaxBTCTripCount = C->getAPInt().getZExtValue() + 1; // BTC + 1
          if (DebugPrints)
            outs() << "    - Max BTC trip count (fallback): " << MaxBTCTripCount
                   << "\n";
        }
      }

      // Prefer the JSON pragma bound over the loose max-BTC estimate. The clang
      // plugin records each bound at the loop *statement* line (e.g. the
      // `while` keyword via getBeginLoc), so match on the loop's start location
      // -- not the header's first body instruction, which sits one line into
      // the loop after rotation.
      if (TripCount == 0 && !JSONBounds.empty()) {
        // Build the basename:line key (same normalization as the JSON map
        // above), look it up, record the trip count on a hit. Returns true on a
        // hit.
        auto TryJSONLookup = [&](const DebugLoc &DL) -> bool {
          if (!DL)
            return false;
          StringRef Base = sys::path::filename(DL->getFilename());
          std::string Key = (Base + ":" + std::to_string(DL->getLine())).str();
          auto It = JSONBoundsByLocation.find(Key);
          if (It != JSONBoundsByLocation.end()) {
            TripCount = It->second;
            if (DebugPrints)
              outs() << "    - Got trip count from JSON: " << TripCount
                     << " (location: " << Key << ")\n";
            return true;
          }
          if (DebugPrints)
            outs() << "    - No JSON bound found for location: " << Key << "\n";
          return false;
        };

        // Primary: the loop's start location (from !llvm.loop metadata) lines
        // up with the plugin's recorded line. Fall back to the header's first
        // non-PHI instruction for loops lacking loop-start debug info.
        if (!TryJSONLookup(L->getStartLoc())) {
          for (const Instruction &I : *BB) {
            if (!isa<PHINode>(I)) {
              TryJSONLookup(I.getDebugLoc());
              break; // Only check the first non-PHI instruction
            }
          }
        }
      }
    }

    // Machine-block pragma fallback. Reached when no clean IR loop was
    // available (so the SCEV / start-location path above never ran) or it
    // produced no bound. Match a JSON pragma against the source lines covered
    // by this loop's *own* machine blocks, excluding nested sub-loops so an
    // outer loop never grabs an inner loop's bound. Pick the smallest matching
    // line: a loop's own statement (the `for`/`while` the plugin annotated)
    // sits at the lexically-first source line of its region. This recovers
    // bounds for rotated loops whose machine header no longer maps to the IR
    // loop header, and attaches them to the genuine machine-loop header the ILP
    // uses.
    if (TripCount == 0 && !JSONBounds.empty()) {
      unsigned BestLine = 0;
      unsigned BestBound = 0;
      for (MachineBasicBlock *MB : ML->getBlocks()) {
        if (MLI.getLoopFor(MB) != ML)
          continue; // skip blocks owned by a nested sub-loop
        for (const MachineInstr &MI : *MB) {
          const DebugLoc &DL = MI.getDebugLoc();
          if (!DL)
            continue;
          StringRef Base = sys::path::filename(DL->getFilename());
          std::string Key = (Base + ":" + std::to_string(DL->getLine())).str();
          auto It = JSONBoundsByLocation.find(Key);
          if (It == JSONBoundsByLocation.end())
            continue;
          if (BestLine == 0 || DL->getLine() < BestLine) {
            BestLine = DL->getLine();
            BestBound = It->second;
          }
        }
      }
      if (BestBound > 0) {
        TripCount = BestBound;
        if (DebugPrints)
          outs() << "    - Got trip count from JSON (machine-block scan): "
                 << TripCount << " (line " << BestLine << ")\n";
      }
    }

    // Absolute last resort: the pessimistic SCEV max-BTC bound, applied only
    // when neither an exact SCEV trip count, a start-location JSON match, nor
    // the machine-block JSON scan produced a (tighter) bound. The loose max-BTC
    // must never shadow a JSON pragma -- including one matched via the
    // machine-block scan (e.g. a goto-formed loop whose header start-location
    // resolves to the pre-header line rather than the annotated statement, so
    // only the machine-block scan finds the pragma).
    if (TripCount == 0 && MaxBTCTripCount > 0) {
      TripCount = MaxBTCTripCount;
      if (DebugPrints)
        outs() << "    - Using max-BTC fallback trip count: " << TripCount
               << "\n";
    }

    // Final seam: a backend-synthesized loop with no source statement and no IR
    // loop (e.g. an MSP430 multi-bit shift loop) carries no SCEV/pragma bound.
    // Let the target recognize and bound it. Queried last so it never shadows a
    // tighter bound.
    if (TripCount == 0) {
      if (std::optional<unsigned> ImplicitBound =
              TAR.getTarget().getImplicitLoopBound(*ML)) {
        TripCount = *ImplicitBound;
        if (DebugPrints)
          outs() << "    - Got trip count from target implicit-loop bound: "
                 << TripCount << "\n";
      }
    }

    if (TripCount > 0) {
      LoopBounds[Header] = TripCount;
      LLVM_DEBUG(dbgs() << "Loop bound for MBB " << Header->getNumber() << " ("
                        << Header->getName() << "): " << TripCount << "\n");
    }
  }

  // Irreducible loops MachineLoopInfo did not recognize (multi-entry loops,
  // e.g. a switch jump-table forming Duff's device). MachineLoopInfo reports no
  // loop, so the per-machine-loop scan above never bounded them. Find their
  // backedges via a DFS retreating-edge scan, bound the header from a JSON
  // pragma matched against the loop body's source lines, and record the
  // backedge so ProgramGraph flags the loop-closing predecessor (which makes
  // the IPET loop-bound row fire for the now-bounded header).
  if (!JSONBounds.empty()) {
    for (const auto &[B, H] : findBackEdges(F)) {
      // Skip reducible loops: MachineLoopInfo already exposes their header, so
      // the scan above handled them. Only headers MLI does not recognize fall
      // through to this irreducible path.
      MachineLoop *HL = MLI.getLoopFor(H);
      if (HL && HL->getHeader() == H)
        continue;
      if (LoopBounds.count(H))
        continue; // already bounded as an irreducible header

      auto Body = computeLoopBody(const_cast<MachineBasicBlock *>(H));
      unsigned BestLine = 0;
      unsigned BestBound = 0;
      for (const MachineBasicBlock *MB : Body) {
        for (const MachineInstr &MI : *MB) {
          const DebugLoc &DL = MI.getDebugLoc();
          if (!DL)
            continue;
          StringRef Base = sys::path::filename(DL->getFilename());
          std::string Key = (Base + ":" + std::to_string(DL->getLine())).str();
          auto It = JSONBoundsByLocation.find(Key);
          if (It == JSONBoundsByLocation.end())
            continue;
          if (BestLine == 0 || DL->getLine() < BestLine) {
            BestLine = DL->getLine();
            BestBound = It->second;
          }
        }
      }
      if (BestBound > 0) {
        LoopBounds[H] = BestBound;
        TAR.addIrreducibleBackEdge(B, H);
        if (DebugPrints)
          outs() << "  Irreducible loop: header MBB " << H->getNumber()
                 << " bounded at " << BestBound << " (line " << BestLine
                 << "), backedge from MBB " << B->getNumber() << "\n";
      }
    }
  }

  // Merge with existing bounds if any (though this pass runs once per function,
  // TAR is global/shared?) TAR seems to be passed around. If we run this pass
  // on multiple functions, we should accumulate. But TAR.LoopBoundMap is a map.
  // We should probably fetch existing map, update it, and set it back.

  auto ExistingBounds = TAR.getLoopBoundMap();
  ExistingBounds.insert(LoopBounds.begin(), LoopBounds.end());
  TAR.setLoopBoundMap(ExistingBounds);
  if (DebugPrints)
    outs() << "MachineLoopBoundAgregatorPass: Found " << LoopBounds.size()
           << " loop bounds in function " << F.getName() << "\n";
  for (auto const &[MBB, Bound] : LoopBounds) {
    if (DebugPrints)
      outs() << "  MBB " << MBB->getNumber() << " (" << MBB->getName()
             << "): " << Bound << "\n";
  }

  return false;
}

MachineFunctionPass *
createMachineLoopBoundAgregatorPass(TimingAnalysisResults &TAR) {
  return new MachineLoopBoundAgregatorPass(TAR);
}

} // namespace llvm
