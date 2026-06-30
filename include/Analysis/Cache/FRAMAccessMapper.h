#ifndef ANALYSIS_CACHE_FRAM_ACCESS_MAPPER_H
#define ANALYSIS_CACHE_FRAM_ACCESS_MAPPER_H

#include "Analysis/Cache/CacheAccessMapper.h"
#include "Analysis/Cache/CacheGeometry.h"

#include <cstdint>
#include <unordered_map>

namespace llvm {

class MachineInstr;
class TimingAnalysisResults;

/// MSP430 FRAM specialization of CacheAccessMapper.
///
/// For each instruction it emits, in order:
///   1. one Access per 16-bit instruction-fetch word whose address is in the
///      FRAM region (>= FRAMStart). Words below FRAMStart (SRAM-resident code,
///      unusual) are not cached and emit nothing.
///   2. for an explicit data memory access (mayLoad/mayStore): nothing if it is
///      provably a stack (SRAM) access, otherwise a Barrier carrying the FRAM
///      data-access wait-state cost — an FRAM/unknown data access both incurs a
///      real wait state (counted into the WCET) and shares the cache so it may
///      evict a fetch line, so conservatively wiping the abstract state keeps
///      the fetch-hit classification sound.
///
/// Which data accesses are non-wait-state memory (SRAM/stack) and which are FRAM
/// is decided from the instruction's MachineMemOperands and target-independent
/// IR-object/address resolution (see Utility/DataMemoryAccess.h). The
/// per-function fetch word counts (\p Words) and the per-instruction FRAM
/// data-access word counts (\p DataWords) are both precomputed by the caller
/// (see Utility/InstructionWords.h); a MachineInstr missing from \p DataWords
/// makes no charged FRAM data access. \p DataAccessCost is the wait-state
/// penalty charged per FRAM data-access word.
class FRAMAccessMapper : public CacheAccessMapper {
public:
  FRAMAccessMapper(const TimingAnalysisResults &TAR, CacheGeometry Geo,
                   const std::unordered_map<const MachineInstr *, unsigned> &Words,
                   const std::unordered_map<const MachineInstr *, unsigned> &DataWords,
                   unsigned DataAccessCost = 0)
      : TAR(TAR), Geo(Geo), Words(Words), DataWords(DataWords),
        DataAccessCost(DataAccessCost) {}

  void mapEvents(const MachineInstr *MI,
                 SmallVectorImpl<CacheEvent> &Out) override;

private:
  const TimingAnalysisResults &TAR;
  CacheGeometry Geo;
  const std::unordered_map<const MachineInstr *, unsigned> &Words;
  const std::unordered_map<const MachineInstr *, unsigned> &DataWords;
  unsigned DataAccessCost;
};

} // namespace llvm

#endif // ANALYSIS_CACHE_FRAM_ACCESS_MAPPER_H
