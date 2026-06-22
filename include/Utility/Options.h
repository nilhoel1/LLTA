#ifndef UTIL_OPTIONS_H
#define UTIL_OPTIONS_H

#include "llvm/Support/CommandLine.h"

/**
 * Path to the linked ELF executable for address resolution (replaces the former
 * -dump-file). Drives address resolution and library-call (ABI) cost decoding
 * via llvm::object::ObjectFile + MCDisassembler. Optional: when empty, the
 * analysis runs but its result is reported as UNSOUND.
 */
extern llvm::cl::opt<std::string> ElfFilename;

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
 * Verbose diagnostics for the address resolver (-address-resolver-verbose).
 */
extern llvm::cl::opt<bool> AddressResolverVerbose;

// NOTE: MSP430(FR)-specific options (-fram-*) are owned by the MSP430 target;
// see include/Targets/MSP430/MSP430Options.h.

#endif
