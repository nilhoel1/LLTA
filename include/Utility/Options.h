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

#endif
