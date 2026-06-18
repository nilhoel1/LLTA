#ifndef UTIL_OPTIONS_H
#define UTIL_OPTIONS_H

#include "llvm/Support/CommandLine.h"

/**
 * Path to the dump file for Adress Resolving
 */
extern llvm::cl::opt<std::string> DumpFilename;

extern llvm::cl::opt<std::string> StartFunctionName;

extern llvm::cl::opt<bool> DebugIR;

extern llvm::cl::opt<std::string> LoopBoundsJSON;

/**
 * ILP solver selection: "auto", "gurobi", or "highs"
 */
extern llvm::cl::opt<std::string> ILPSolverOption;

/**
 * Run purely as a compiler driver (like llc)
 */
extern llvm::cl::opt<bool> LLCMode;

/**
 * Hex start address of the MSP430 FRAM region (foundation only; -fram-start).
 */
extern llvm::cl::opt<std::string> FRAMStartAddress;

/**
 * Verbose diagnostics for the address resolver (-address-resolver-verbose).
 */
extern llvm::cl::opt<bool> AddressResolverVerbose;

/**
 * Number of MSP430 FRAM wait states added per FRAM instruction-fetch access
 * (-fram-wait-states). 0 disables the FRAM wait-state model (default).
 */
extern llvm::cl::opt<unsigned> FRAMWaitStates;

/**
 * Enable the FRAM read-cache must-analysis (-fram-cache). When set, the
 * cache-aware FRAMCacheAnalysisPass replaces the no-cache FRAMWaitStatePass for
 * the FRAM fetch penalty. Requires -fram-wait-states > 0 and -fram-start.
 */
extern llvm::cl::opt<bool> FRAMCache;

/**
 * FRAM cache replacement-policy module (-fram-cache-policy): "unknown"
 * (adversarial; sound for the undocumented FR5994 policy, default), "lru" or
 * "fifo" (age-based; tighter but sound only if the device matches that policy).
 */
extern llvm::cl::opt<std::string> FRAMCachePolicy;

/**
 * Verbose FRAM cache diagnostics (-fram-cache-verbose). Additionally runs a
 * sound may-analysis and reports accesses proven never cached ("always-miss").
 * Does not change the WCET. Off by default.
 */
extern llvm::cl::opt<bool> FRAMCacheVerbose;

/** Number of FRAM cache sets (-fram-cache-sets). FR5994 default: 2. */
extern llvm::cl::opt<unsigned> FRAMCacheSets;

/** FRAM cache associativity (-fram-cache-ways); used by the "lru" policy.
 *  FR5994 default: 2. */
extern llvm::cl::opt<unsigned> FRAMCacheWays;

/** FRAM cache line size in bytes (-fram-cache-line-bytes). FR5994 default: 8. */
extern llvm::cl::opt<unsigned> FRAMCacheLineBytes;

#endif
