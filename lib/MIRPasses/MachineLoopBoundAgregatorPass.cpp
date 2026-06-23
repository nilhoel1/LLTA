#include "MIRPasses/MachineLoopBoundAgregatorPass.h"
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
#include <string>
#include <unordered_map>

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
      unsigned MaxBTCTripCount = 0;
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

      // Last resort: fall back to the pessimistic SCEV max-BTC bound only when
      // neither an exact SCEV trip count nor a JSON bound was available.
      if (TripCount == 0 && MaxBTCTripCount > 0) {
        TripCount = MaxBTCTripCount;
        if (DebugPrints)
          outs() << "    - Using max-BTC fallback trip count: " << TripCount
                 << "\n";
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

    if (TripCount > 0) {
      LoopBounds[Header] = TripCount;
      LLVM_DEBUG(dbgs() << "Loop bound for MBB " << Header->getNumber() << " ("
                        << Header->getName() << "): " << TripCount << "\n");
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
