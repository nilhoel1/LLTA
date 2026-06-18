#include "Targets/MSP430/MSP430Options.h"

using namespace llvm;

// Target-specific options for the MSP430(FR) family. Registered in their own
// option category so they group separately from the generic LLTA options.
static cl::OptionCategory MSP430Cat("1. MSP430 Target Options");

cl::opt<std::string> FRAMStartAddress(
    "fram-start", cl::init(""),
    cl::desc("Hex start address of the MSP430 FRAM region, e.g. 0x4000. "
             "Identifies the FRAM region for the FRAM fetch-penalty models."),
    cl::cat(MSP430Cat));

cl::opt<unsigned> FRAMWaitStates(
    "fram-wait-states", cl::init(0),
    cl::desc("MSP430 FRAM wait states added per FRAM instruction-fetch access "
             "(one access per 16-bit code word). 0 disables the model "
             "(default). Typical FR5994 values: <=8 MHz -> 0, >8-16 MHz -> 1, "
             ">16-24 MHz -> 2. Requires -fram-start to identify the FRAM "
             "region."),
    cl::cat(MSP430Cat));

cl::opt<bool> FRAMCache(
    "fram-cache", cl::init(false),
    cl::desc("Enable the MSP430 FRAM read-cache must-analysis. The cache-aware "
             "fetch penalty (a sound refinement of -fram-wait-states) replaces "
             "the no-cache FRAMWaitStatePass. Requires -fram-wait-states > 0 "
             "and -fram-start."),
    cl::cat(MSP430Cat));

cl::opt<std::string> FRAMCachePolicy(
    "fram-cache-policy", cl::init("unknown"),
    cl::desc("FRAM cache replacement-policy module: 'unknown' (adversarial, "
             "sound for the undocumented FR5994 policy; default), 'lru' or "
             "'fifo' (age-based; tighter but sound only if the device matches "
             "that policy)."),
    cl::cat(MSP430Cat));

cl::opt<bool> FRAMCacheVerbose(
    "fram-cache-verbose", cl::init(false),
    cl::desc("Additionally run a sound FRAM may-analysis and report accesses "
             "proven never cached ('always-miss'). Diagnostic only; does not "
             "change the WCET. Requires -fram-cache."),
    cl::cat(MSP430Cat));

cl::opt<unsigned> FRAMCacheSets("fram-cache-sets", cl::init(2),
                                cl::desc("FRAM cache number of sets (FR5994: 2)."),
                                cl::cat(MSP430Cat));

cl::opt<unsigned> FRAMCacheWays(
    "fram-cache-ways", cl::init(2),
    cl::desc("FRAM cache associativity, used by the 'lru' policy (FR5994: 2)."),
    cl::cat(MSP430Cat));

cl::opt<unsigned> FRAMCacheLineBytes(
    "fram-cache-line-bytes", cl::init(8),
    cl::desc("FRAM cache line size in bytes (FR5994: 8)."), cl::cat(MSP430Cat));
