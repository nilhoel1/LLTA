#ifndef TIMING_ANALYSIS_RESULTS_H
#define TIMING_ANALYSIS_RESULTS_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include <unordered_map>
#include "RTTargets/MuArchStateGraph.h"
namespace llvm {

//===- TimingAnalysisResults.h - Timing Analysis Results -------*- C++ -*-===//
// This class amkes all Timing Analysis results available between TimingAnalysisPasses.
// It is used to store the results of the Timing Analysis passes.
class TimingAnalysisResults {
public:

    // START: Instruction Latency Pass Containers
    std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap;
    bool MBBLatencyMapSet = false;

    void setMBBLatencyMap(std::unordered_map<const MachineBasicBlock *, unsigned int> MBBLatencyMap);

    std::unordered_map<const MachineBasicBlock *, unsigned int> getMBBLatencyMap();
    // END: Instruction Latency Pass Containers

    // START: Machine Loop Bound Agregator Pass Containers
    std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBoundMap;
    bool LoopBoundMapSet = false;

    void setLoopBoundMap(std::unordered_map<const MachineBasicBlock *, unsigned int> LoopBoundMap);

    std::unordered_map<const MachineBasicBlock *, unsigned int> getLoopBoundMap();
    // END: Machine Loop Bound Agregator Pass Containers

    // START: MuArchStateGraph Container
    MuArchStateGraph MASG;
    // END: MuArchStateGraph Container
};

} // namespace llvm

#endif // TIMING_ANALYSIS_RESULTS_H
