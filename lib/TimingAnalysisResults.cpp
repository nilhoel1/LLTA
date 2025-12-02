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

} // namespace llvm
