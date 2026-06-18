#include "Utility/Options.h"

using namespace llvm;

static cl::OptionCategory LLTA("0. LLTA Options");

cl::opt<std::string> DumpFilename("dump-file", cl::init("-"),
                                  cl::desc("Input dump file"), cl::cat(LLTA));

cl::opt<std::string> StartFunctionName(
    "start-function", cl::init(""),
    cl::desc("Name of the functions to start the timing analysis from"),
    cl::cat(LLTA));

cl::opt<bool>
    DebugIR("gIR", cl::init(false),
            cl::desc("Use this option to move debug information for the IR"),
            cl::cat(LLTA));

cl::opt<std::string> LoopBoundsJSON(
    "loop-bounds-json", cl::init(""),
    cl::desc("Path to JSON file containing loop bounds from clang plugin"),
    cl::cat(LLTA));

cl::opt<std::string>
    ILPSolverOption("ilp-solver", cl::init("auto"),
                    cl::desc("Abstract ILP solver backend for WCET "
                             "calculation: 'auto' (default), 'gurobi', or "
                             "'highs'. With 'auto', Gurobi is tried first if "
                             "available and licensed, then HiGHS."),
                    cl::cat(LLTA));

cl::opt<bool>
    LLCMode("llc", cl::init(false),
            cl::desc("Run purely as a compiler driver (like llc) executing "
                     "transformation passes without performing WCET analysis."),
            cl::cat(LLTA));

cl::opt<bool> AddressResolverVerbose(
    "address-resolver-verbose", cl::init(false),
    cl::desc("Print detailed diagnostics from the address resolver: every "
             "encoding cross-check mismatch and offset repair, with the "
             "function, instruction, expected vs. actual bytes and assembly."),
    cl::cat(LLTA));


// MSP430(FR)-specific options (-fram-*) are owned by the MSP430 target:
// lib/Targets/MSP430/MSP430Options.cpp.
