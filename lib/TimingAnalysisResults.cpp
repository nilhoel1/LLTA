#include "TimingAnalysisResults.h"

#include "Targets/RTTarget.h"

#include <cassert>
#include <utility>

namespace llvm {

// Defined out-of-line so the unique_ptr<llta::RTTarget> member can be held with
// an incomplete type in the header.
TimingAnalysisResults::TimingAnalysisResults() = default;
TimingAnalysisResults::~TimingAnalysisResults() = default;

// START: Active Target
void TimingAnalysisResults::setTarget(std::unique_ptr<llta::RTTarget> Target) {
  ActiveTarget = std::move(Target);
}

const llta::RTTarget &TimingAnalysisResults::getTarget() const {
  assert(ActiveTarget &&
         "active target not set; setTarget() must run before any pass");
  return *ActiveTarget;
}
// END: Active Target

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

void TimingAnalysisResults::addIrreducibleBackEdge(
    const MachineBasicBlock *Pred, const MachineBasicBlock *Header) {
  IrreducibleBackEdges.insert({Pred, Header});
}

const std::set<
    std::pair<const MachineBasicBlock *, const MachineBasicBlock *>> &
TimingAnalysisResults::getIrreducibleBackEdges() const {
  return IrreducibleBackEdges;
}
// END: Machine Loop Bound Agregator Pass Containers

// START: Address Resolver Pass Containers
void TimingAnalysisResults::setInstructionAddress(const MachineInstr *MI,
                                                  uint64_t Address) {
  InstructionAddressMapSet = true;
  InstructionAddressMap[MI] = Address;
}

bool TimingAnalysisResults::hasInstructionAddress(
    const MachineInstr *MI) const {
  return InstructionAddressMap.count(MI) != 0;
}

uint64_t
TimingAnalysisResults::getInstructionAddress(const MachineInstr *MI) const {
  auto It = InstructionAddressMap.find(MI);
  assert(It != InstructionAddressMap.end() &&
         "No address resolved for this MachineInstr");
  return It->second;
}

void TimingAnalysisResults::setBranchTarget(const MachineInstr *MI,
                                            uint64_t Target) {
  BranchTargetMapSet = true;
  BranchTargetMap[MI] = Target;
}

bool TimingAnalysisResults::hasBranchTarget(const MachineInstr *MI) const {
  return BranchTargetMap.count(MI) != 0;
}

uint64_t TimingAnalysisResults::getBranchTarget(const MachineInstr *MI) const {
  auto It = BranchTargetMap.find(MI);
  assert(It != BranchTargetMap.end() &&
         "No branch target resolved for this MachineInstr");
  return It->second;
}

void TimingAnalysisResults::addDataObject(DataObject Obj) {
  auto It = DataObjectByName.find(Obj.Name);
  if (It != DataObjectByName.end()) {
    // Duplicate symbol name (e.g. local statics from different units); keep the
    // first and skip the rest so the name lookup stays deterministic.
    return;
  }
  DataObjectByName[Obj.Name] = DataObjects.size();
  DataObjects.push_back(std::move(Obj));
}

const TimingAnalysisResults::DataObject *
TimingAnalysisResults::getDataObject(StringRef Name) const {
  auto It = DataObjectByName.find(Name.str());
  if (It == DataObjectByName.end())
    return nullptr;
  return &DataObjects[It->second];
}

const std::vector<TimingAnalysisResults::DataObject> &
TimingAnalysisResults::getDataObjects() const {
  return DataObjects;
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

// START: Unsoundness tracking
void TimingAnalysisResults::addUnsoundReason(StringRef Reason) {
  UnsoundReasons.insert(Reason.str());
}

const std::set<std::string> &TimingAnalysisResults::getUnsoundReasons() const {
  return UnsoundReasons;
}

bool TimingAnalysisResults::isUnsound() const {
  return !UnsoundReasons.empty();
}
// END: Unsoundness tracking

} // namespace llvm
