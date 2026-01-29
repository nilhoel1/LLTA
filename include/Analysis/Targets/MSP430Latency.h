#ifndef MSP430_LATENCY_H
#define MSP430_LATENCY_H

#include "llvm/CodeGen/MachineInstr.h"

namespace llta {
namespace targets {

unsigned getMSP430Latency(const llvm::MachineInstr &I);

} // namespace targets
} // namespace llta

#endif // MSP430_LATENCY_H
