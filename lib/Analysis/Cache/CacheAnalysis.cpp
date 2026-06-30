#include "Analysis/Cache/CacheAnalysis.h"
#include "Analysis/Cache/CacheEvent.h"
#include "Analysis/Cache/CacheState.h"

#include "llvm/ADT/SmallVector.h"

namespace llvm {

std::unique_ptr<AbstractState> CacheAnalysis::getInitialState() {
  // Cold cache: every set empty. Sound for both directions (must: nothing
  // guaranteed; may: nothing possibly-cached yet).
  return std::make_unique<CacheState>(Geo, Policy, Kind);
}

unsigned CacheAnalysis::process(AbstractState *State, const MachineInstr *MI) {
  auto *CState = static_cast<CacheState *>(State);

  SmallVector<CacheEvent, 4> Events;
  Mapper->mapEvents(MI, Events);

  unsigned Cost = 0;
  for (const CacheEvent &E : Events) {
    switch (E.Kind) {
    case CacheEvent::Access: {
      bool Present = CState->access(E.LineId);
      if (Kind == AnalysisKind::Must) {
        if (!Present)
          Cost += MissPenalty; // not provably a hit ⇒ charge the miss
      } else {                 // May
        if (!Present && Sink)
          Sink(MI, E.LineId);  // provably not cached ⇒ guaranteed miss
      }
      break;
    }
    case CacheEvent::Barrier:
      CState->barrier();
      if (Kind == AnalysisKind::Must)
        Cost += E.Cost; // FRAM data-access wait state(s); May cost stays 0
      break;
    }
  }
  return Cost;
}

} // namespace llvm
