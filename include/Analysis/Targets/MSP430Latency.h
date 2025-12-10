#ifndef LLTA_ANALYSIS_TARGETS_MSP430LATENCY_H
#define LLTA_ANALYSIS_TARGETS_MSP430LATENCY_H

#include "llvm/CodeGen/MachineInstr.h"

namespace llta {
namespace targets {

/// Returns the latency of an MSP430 instruction in cycles.
/// This logic assumes the MSP430 has no pipeline and latencies are fixed.
unsigned getMSP430Latency(const llvm::MachineInstr &I);

} // namespace targets
} // namespace llta

#endif // LLTA_ANALYSIS_TARGETS_MSP430LATENCY_H
