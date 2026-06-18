#ifndef TIMING_ANALYSIS_RESULTS_H
#define TIMING_ANALYSIS_RESULTS_H

#include "Graph/ProgramGraph.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace llta {
class RTTarget;
} // namespace llta

namespace llvm {

class MachineInstr;

//===- TimingAnalysisResults.h - Timing Analysis Results -------*- C++ -*-===//
// This class amkes all Timing Analysis results available between
// TimingAnalysisPasses. It is used to store the results of the Timing Analysis
// passes.
class TimingAnalysisResults {
public:
  TimingAnalysisResults();
  ~TimingAnalysisResults();

  // START: Active Target
  // The timing-analysis target (a concrete device) that describes the
  // architecture under analysis. Resolved once from the LLVM Triple (via the
  // target registry) when the pass pipeline is built, then queried by generic
  // passes instead of switching on the arch.
  std::unique_ptr<llta::RTTarget> ActiveTarget;

  /// Install the active target (takes ownership). Called once while building
  /// the pass pipeline, before any pass runs.
  void setTarget(std::unique_ptr<llta::RTTarget> Target);

  /// Return the active target. Asserts setTarget() was called first.
  const llta::RTTarget &getTarget() const;
  // END: Active Target

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

  // Static branch/call target of each control-flow MachineInstr, recovered from
  // the dump's trailing comment (";abs 0x...." for jumps, ";#0x...." for calls)
  // by AdressResolverPass. Indirect transfers have no entry.
  std::unordered_map<const MachineInstr *, uint64_t> BranchTargetMap;
  bool BranchTargetMapSet = false;

  void setBranchTarget(const MachineInstr *MI, uint64_t Target);
  bool hasBranchTarget(const MachineInstr *MI) const;
  uint64_t getBranchTarget(const MachineInstr *MI) const;

  // Data/heap objects discovered in the dump's data sections (.data, .bss,
  // .rodata, .heap, ...) by AdressResolverPass. Foundation only; no timing pass
  // consumes them yet.
  struct DataObject {
    std::string Name;
    uint64_t Address = 0;
    uint64_t Size = 0; ///< next symbol addr - this addr (0 if last/unknown)
    std::string Section;
  };
  std::vector<DataObject> DataObjects;
  std::unordered_map<std::string, size_t> DataObjectByName; ///< name -> index

  void addDataObject(DataObject Obj);
  const DataObject *getDataObject(StringRef Name) const; ///< nullptr if absent
  const std::vector<DataObject> &getDataObjects() const;

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
