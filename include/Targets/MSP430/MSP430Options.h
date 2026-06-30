#ifndef LLTA_TARGETS_MSP430_MSP430OPTIONS_H
#define LLTA_TARGETS_MSP430_MSP430OPTIONS_H

#include "llvm/Support/CommandLine.h"

// MSP430(FR) target-specific command-line options. These are owned by the
// MSP430 target rather than the generic option set: they only make sense for
// MSP430FR devices (FRAM program memory + FR5xx read cache). Defaults are
// FR5994 values.

/// Hex start address of the MSP430 FRAM region, e.g. 0x4000 (-fram-start).
extern llvm::cl::opt<std::string> FRAMStartAddress;

/// FRAM wait states added per FRAM instruction-fetch access (one access per
/// 16-bit code word). 0 disables the wait-state model (-fram-wait-states).
extern llvm::cl::opt<unsigned> FRAMWaitStates;

/// Enable the FRAM read-cache must-analysis (-fram-cache). When set, the
/// cache-aware FRAMCacheAnalysisPass replaces the no-cache FRAMWaitStatePass
/// for the FRAM fetch penalty. Requires -fram-wait-states > 0 and -fram-start.
extern llvm::cl::opt<bool> FRAMCache;

/// Cycles charged for one FRAM instruction-fetch cache miss / line fill
/// (-fram-line-fill-cycles). Physically distinct from the per-word
/// -fram-wait-states. FR5994 16 MHz default: 15. Used only with -fram-cache.
extern llvm::cl::opt<unsigned> FRAMLineFillCycles;

/// FRAM cache replacement-policy module: "unknown" (adversarial; sound for the
/// undocumented FR5994 policy, default), "lru" or "fifo" (-fram-cache-policy).
extern llvm::cl::opt<std::string> FRAMCachePolicy;

/// Verbose FRAM cache diagnostics (-fram-cache-verbose): also run a sound
/// may-analysis and report accesses proven never cached. Does not change WCET.
extern llvm::cl::opt<bool> FRAMCacheVerbose;

/// Number of FRAM cache sets (-fram-cache-sets). FR5994 default: 2.
extern llvm::cl::opt<unsigned> FRAMCacheSets;

/// FRAM cache associativity (-fram-cache-ways); used by "lru". FR5994: 2.
extern llvm::cl::opt<unsigned> FRAMCacheWays;

/// FRAM cache line size in bytes (-fram-cache-line-bytes). FR5994: 8.
extern llvm::cl::opt<unsigned> FRAMCacheLineBytes;

#endif // LLTA_TARGETS_MSP430_MSP430OPTIONS_H
