#include "MIRPasses/MachineLoopBoundAgregatorPass.h"
#include "Utility/Options.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <unordered_map>

#define DEBUG_TYPE "machine-loop-bound-aggregator"

namespace llvm {

// Helper structure to store loop bound data from JSON
struct JSONLoopBound {
  std::string FileName;
  unsigned Line;
  unsigned Column;
  unsigned LowerBound;
  unsigned UpperBound;
};

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
    std::string Key = Bound.FileName + ":" + std::to_string(Bound.Line);
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

    if (!BB) {
      outs() << " - NO IR BasicBlock mapped!\n";
      continue;
    }

    if (DebugPrints)
      outs() << " maps to IR BB " << BB->getName() << "\n";

    Loop *L = LI.getLoopFor(BB);
    if (!L) {
      outs() << "    - No IR Loop found for this BB\n";
      continue;
    }

    // Check if the IR loop header matches the Machine loop header's BB
    // This ensures we are looking at the same loop structure
    if (L->getHeader() != BB) {
      outs() << "    - IR Loop header mismatch: IR loop header is "
             << L->getHeader()->getName() << "\n";
      continue;
    }
    if (DebugPrints)
      outs() << "    - Found matching IR loop\n";

    unsigned TripCount = 0;

    // First try to get trip count from SCEV
    TripCount = SE.getSmallConstantTripCount(L);
    // getSmallConstantTripCount returns 0 if unknown or not constant.
    // It also returns the exact trip count, not the bound.
    // But for timing analysis, exact trip count is often what we want if it's
    // constant. If it's not constant, we might want max backedge taken count.
    if (DebugPrints)
      outs() << "    - SmallConstantTripCount: " << TripCount << "\n";

    if (TripCount == 0) {
      // Try to get max backedge taken count
      const SCEV *MaxBTC = SE.getConstantMaxBackedgeTakenCount(L);
      if (DebugPrints)
        outs() << "    - Trying max backedge taken count: " << *MaxBTC << "\n";
      if (auto *C = dyn_cast<SCEVConstant>(MaxBTC)) {
        TripCount = C->getAPInt().getZExtValue() + 1; // Trip count is BTC + 1
        if (DebugPrints)
          outs() << "    - Got trip count from max BTC: " << TripCount << "\n";
      }
    }

    // If SCEV failed, try to use JSON bounds
    if (TripCount == 0 && !JSONBounds.empty()) {
      // Get the debug location of the loop header
      for (const Instruction &I : *BB) {
        if (!isa<PHINode>(I)) {
          if (const DebugLoc &DL = I.getDebugLoc()) {
            std::string FileName = DL->getFilename().str();
            unsigned Line = DL->getLine();

            // Extract just the filename without path
            size_t LastSlash = FileName.find_last_of("/\\");
            if (LastSlash != std::string::npos) {
              FileName = FileName.substr(LastSlash + 1);
            }

            std::string Key = FileName + ":" + std::to_string(Line);
            auto It = JSONBoundsByLocation.find(Key);
            if (It != JSONBoundsByLocation.end()) {
              TripCount = It->second;
              if (DebugPrints)
                outs() << "    - Got trip count from JSON: " << TripCount
                       << " (location: " << Key << ")\n";
            } else {
              if (DebugPrints)
                outs() << "    - No JSON bound found for location: " << Key
                       << "\n";
            }
          }
          break; // Only check the first non-PHI instruction
        }
      }
    }
    if (TripCount > 0) {
      LoopBounds[Header] = TripCount;
      LLVM_DEBUG(dbgs() << "Loop bound for MBB " << Header->getNumber()
                        << " (IR " << BB->getName() << "): " << TripCount
                        << "\n");
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
