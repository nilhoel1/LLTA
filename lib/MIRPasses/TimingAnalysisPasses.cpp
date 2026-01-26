#include "MIRPasses/TimingAnalysisPasses.h"
#include "MIRPasses/AccessAnalyses.h"
#include "MIRPasses/AdressResolverPass.h"
#include "MIRPasses/AsmDumpAndCheckPass.h"
#include "MIRPasses/CallSplitterPass.h"
#include "MIRPasses/DebugIRPass.h"
#include "MIRPasses/FillMuGraphPass.h"
#include "MIRPasses/InstructionLatencyPass.h"
#include "MIRPasses/MachineLoopBoundAgregatorPass.h"
#include "MIRPasses/Mir2IrPass.h"
#include "MIRPasses/PathAnalysisPass.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"

namespace llvm {

// Container that holds the results of the timing analysis passes
// and makes them available to all timing analysis passes.
static TimingAnalysisResults TAR = TimingAnalysisResults();

std::list<MachineFunctionPass *> getTimingAnalysisPasses() {
  std::list<MachineFunctionPass *> Passes;
  Passes.push_back(createCallSplitterPass(TAR));
  if (DebugIR) {
    Passes.push_back(createDebugIRPass());
  }
  Passes.push_back(createAsmDumpAndCheckPass(TAR));
  Passes.push_back(createAdressResolverPass(TAR));
  Passes.push_back(createInstructionLatencyPass(TAR));
  // Passes.push_back(createAccessAnalysesPass(TM));
  Passes.push_back(createMachineLoopBoundAgregatorPass(TAR));
  Passes.push_back(createFillMuGraphPass(TAR));
  Passes.push_back(createPathAnalysisPass(TAR));
  Passes.push_back(createMIRtoIRPass(TAR));
  return Passes;
}

} // namespace llvm
