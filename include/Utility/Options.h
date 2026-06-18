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

#endif
