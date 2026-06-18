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
///      provably a stack (SRAM) access, otherwise a Barrier — an FRAM/unknown
///      data read shares the cache and may evict a fetch line, so conservatively
///      wiping the abstract state keeps the fetch-hit classification sound.
///
/// Stack vs. non-stack is decided from the instruction's MachineMemOperands
/// (Stack / FixedStack pseudo-source values), which is robust and
/// target-independent. The per-function fetch word counts are supplied by the
/// caller (see Utility/InstructionWords.h).
class FRAMAccessMapper : public CacheAccessMapper {
public:
  FRAMAccessMapper(const TimingAnalysisResults &TAR, CacheGeometry Geo,
                   const std::unordered_map<const MachineInstr *, unsigned> &Words)
      : TAR(TAR), Geo(Geo), Words(Words) {}

  void mapEvents(const MachineInstr *MI,
                 SmallVectorImpl<CacheEvent> &Out) override;

private:
  const TimingAnalysisResults &TAR;
  CacheGeometry Geo;
  const std::unordered_map<const MachineInstr *, unsigned> &Words;
};

} // namespace llvm

#endif // ANALYSIS_CACHE_FRAM_ACCESS_MAPPER_H
