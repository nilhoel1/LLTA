#ifndef TIMING_ANALYSIS_RESULTS_H
#define TIMING_ANALYSIS_RESULTS_H

#include "RTTargets/ProgramGraph.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <cstdint>
#include <unordered_map>
namespace llvm {

class MachineInstr;

//===- TimingAnalysisResults.h - Timing Analysis Results -------*- C++ -*-===//
// This class amkes all Timing Analysis results available between
// TimingAnalysisPasses. It is used to store the results of the Timing Analysis
// passes.
class TimingAnalysisResults {
public:
  // START: Instruction Latency Pass Containers
  std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap;
  bool MBBLatencyMapSet = false;

  void
  setMBBLatencyMap(std::unordered_map<const MachineBasicBlock *, unsigned int>
                       MBBLatencyMap);

  std::unordered_map<const MachineBasicBlock *, unsigned int>
  getMBBLatencyMap();
  // END: Instruction Latency Pass Containers

  // START: Machine Loop Bound Agregator Pass Containers
  std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBoundMap;
  bool LoopBoundMapSet = false;

  void setLoopBoundMap(
      std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBoundMap);

  std::unordered_map<const MachineBasicBlock *, unsigned int> getLoopBoundMap();
  // END: Machine Loop Bound Agregator Pass Containers

  // START: Address Resolver Pass Containers
  // Real absolute address of each (code-emitting) MachineInstr, discovered from
  // the objdump dump file by AdressResolverPass.
  std::unordered_map<const MachineInstr *, uint64_t> InstructionAddressMap;
  bool InstructionAddressMapSet = false;

  void setInstructionAddress(const MachineInstr *MI, uint64_t Address);
  bool hasInstructionAddress(const MachineInstr *MI) const;
  uint64_t getInstructionAddress(const MachineInstr *MI) const;

  // Start address of the MSP430 FRAM region, if the user supplied -fram-start.
  // Stored as a foundation only; no timing pass consumes it yet.
  uint64_t FRAMStart = 0;
  bool FRAMStartSet = false;

  void setFRAMStart(uint64_t Address);
  bool hasFRAMStart() const;
  uint64_t getFRAMStart() const;
  // END: Address Resolver Pass Containers

  // START: MuArchStateGraph Container
  ProgramGraph MASG;
  // END: MuArchStateGraph Container
};

} // namespace llvm

#endif // TIMING_ANALYSIS_RESULTS_H
