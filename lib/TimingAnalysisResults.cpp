#include "TimingAnalysisResults.h"

namespace llvm {

// START: Instruction Latency Pass Containers
void TimingAnalysisResults::setMBBLatencyMap(
    std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap) {
  MBBLatencyMapSet = true;
  this->MBBLatencyMap = MBBLatencyMap;
}

std::unordered_map<const MachineBasicBlock *, unsigned int>
TimingAnalysisResults::getMBBLatencyMap() {
  assert(MBBLatencyMapSet && "MBBLatencyMap is not set, and should be set by "
                             "the InstructionLatencyPass");
  return MBBLatencyMap;
}
// END: Instruction Latency Pass Containers

// START: Machine Loop Bound Agregator Pass Containers
void TimingAnalysisResults::setLoopBoundMap(
    std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBoundMap) {
  LoopBoundMapSet = true;
  this->LoopBoundMap = LoopBoundMap;
}

std::unordered_map<const MachineBasicBlock *, unsigned int>
TimingAnalysisResults::getLoopBoundMap() {
  // We do not assert here, because it is possible that no loops are found
  return LoopBoundMap;
}
// END: Machine Loop Bound Agregator Pass Containers

// START: Address Resolver Pass Containers
void TimingAnalysisResults::setInstructionAddress(const MachineInstr *MI,
                                                  uint64_t Address) {
  InstructionAddressMapSet = true;
  InstructionAddressMap[MI] = Address;
}

bool TimingAnalysisResults::hasInstructionAddress(const MachineInstr *MI) const {
  return InstructionAddressMap.count(MI) != 0;
}

uint64_t
TimingAnalysisResults::getInstructionAddress(const MachineInstr *MI) const {
  auto It = InstructionAddressMap.find(MI);
  assert(It != InstructionAddressMap.end() &&
         "No address resolved for this MachineInstr");
  return It->second;
}

void TimingAnalysisResults::setFRAMStart(uint64_t Address) {
  FRAMStartSet = true;
  FRAMStart = Address;
}

bool TimingAnalysisResults::hasFRAMStart() const { return FRAMStartSet; }

uint64_t TimingAnalysisResults::getFRAMStart() const {
  assert(FRAMStartSet && "FRAM start address was not set (-fram-start)");
  return FRAMStart;
}
// END: Address Resolver Pass Containers

} // namespace llvm
