#include "Utility/Options.h"

using namespace llvm;

cl::OptionCategory LLTA("0. LLTA Options");

cl::opt<std::string> DumpFilename(
    "dump-file", cl::init("-"),
    cl::desc("Input dump file"),
    cl::cat(LLTA));

cl::opt<std::string> StartFunctionName(
    "start-function", cl::init(""),
    cl::desc("Name of the functions to start the timing analysis from"),
    cl::cat(LLTA));

cl::opt<bool> DebugIR(
    "gIR", cl::init(false),
    cl::desc("Use this option to move debug information for the IR"),
    cl::cat(LLTA));

cl::opt<std::string> LoopBoundsJSON(
    "loop-bounds-json", cl::init(""),
    cl::desc("Path to JSON file containing loop bounds from clang plugin"),
    cl::cat(LLTA));
