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
                    cl::desc("ILP solver to use for WCET calculation: 'auto' "
                             "(default), 'gurobi', 'highs', or 'all'. "
                             "With 'auto', Gurobi is tried first if available "
                             "and licensed, then HiGHS. "
                             "With 'all', all available solvers are run and "
                             "their performance is compared."),
                    cl::cat(LLTA));

cl::opt<bool>
    LLCMode("llc", cl::init(false),
            cl::desc("Run purely as a compiler driver (like llc) executing "
                     "transformation passes without performing WCET analysis."),
            cl::cat(LLTA));

cl::opt<std::string> FRAMStartAddress(
    "fram-start", cl::init(""),
    cl::desc("Hex start address of the MSP430 FRAM region, e.g. 0x4000. Stored "
             "for later FRAM-aware analysis; does not change timing yet."),
    cl::cat(LLTA));

cl::opt<bool> AddressResolverVerbose(
    "address-resolver-verbose", cl::init(false),
    cl::desc("Print detailed diagnostics from the address resolver: every "
             "encoding cross-check mismatch and offset repair, with the "
             "function, instruction, expected vs. actual bytes and assembly."),
    cl::cat(LLTA));

cl::opt<unsigned> FRAMWaitStates(
    "fram-wait-states", cl::init(0),
    cl::desc("MSP430 FRAM wait states added per FRAM instruction-fetch access "
             "(one access per 16-bit code word). 0 disables the model "
             "(default). Typical FR5994 values: <=8 MHz -> 0, >8-16 MHz -> 1, "
             ">16-24 MHz -> 2. Requires -fram-start to identify the FRAM "
             "region."),
    cl::cat(LLTA));

cl::opt<bool> FRAMCache(
    "fram-cache", cl::init(false),
    cl::desc("Enable the MSP430 FRAM read-cache must-analysis. The cache-aware "
             "fetch penalty (a sound refinement of -fram-wait-states) replaces "
             "the no-cache FRAMWaitStatePass. Requires -fram-wait-states > 0 "
             "and -fram-start."),
    cl::cat(LLTA));

cl::opt<std::string> FRAMCachePolicy(
    "fram-cache-policy", cl::init("unknown"),
    cl::desc("FRAM cache replacement-policy module: 'unknown' (adversarial, "
             "sound for the undocumented FR5994 policy; default), 'lru' or "
             "'fifo' (age-based; tighter but sound only if the device matches "
             "that policy)."),
    cl::cat(LLTA));

cl::opt<bool> FRAMCacheVerbose(
    "fram-cache-verbose", cl::init(false),
    cl::desc("Additionally run a sound FRAM may-analysis and report accesses "
             "proven never cached ('always-miss'). Diagnostic only; does not "
             "change the WCET. Requires -fram-cache."),
    cl::cat(LLTA));

cl::opt<unsigned> FRAMCacheSets("fram-cache-sets", cl::init(2),
                                cl::desc("FRAM cache number of sets (FR5994: 2)."),
                                cl::cat(LLTA));

cl::opt<unsigned> FRAMCacheWays(
    "fram-cache-ways", cl::init(2),
    cl::desc("FRAM cache associativity, used by the 'lru' policy (FR5994: 2)."),
    cl::cat(LLTA));

cl::opt<unsigned> FRAMCacheLineBytes(
    "fram-cache-line-bytes", cl::init(8),
    cl::desc("FRAM cache line size in bytes (FR5994: 8)."), cl::cat(LLTA));
