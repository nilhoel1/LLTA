#include "MIRPasses/TimingAnalysisPasses.h"
#include "MIRPasses/AdressResolverPass.h"
#include "MIRPasses/AsmDumpAndCheckPass.h"
#include "MIRPasses/CallSplitterPass.h"
#include "MIRPasses/FillMuGraphPass.h"
#include "MIRPasses/InstructionLatencyPass.h"
#include "MIRPasses/MachineLoopBoundAgregatorPass.h"
#include "MIRPasses/PathAnalysisPass.h"
#include "Targets/RTTarget.h"
#include "Targets/TargetRegistry.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"

#include "llvm/Support/ErrorHandling.h"

namespace llvm {

// Container that holds the results of the timing analysis passes
// and makes them available to all timing analysis passes.
static TimingAnalysisResults TAR = TimingAnalysisResults();

std::list<MachineFunctionPass *> getTimingAnalysisPasses(const Triple &TT) {
  // Resolve the timing-analysis target once, up front, and install it so every
  // pass can query it (instead of switching on the arch). The target also
  // contributes its own memory-model passes below.
  std::unique_ptr<llta::RTTarget> Tgt = llta::createRTTarget(TT);
  if (!Tgt)
    report_fatal_error("LLTA: no timing-analysis target for this triple");
  TAR.setTarget(std::move(Tgt));
  const llta::RTTarget &Target = TAR.getTarget();

  std::list<MachineFunctionPass *> Passes;
  Passes.push_back(createCallSplitterPass(TAR));
  if (LLCMode)
    return Passes;
  Passes.push_back(createAsmDumpAndCheckPass(TAR));
  Passes.push_back(createAdressResolverPass(TAR));
  Passes.push_back(createInstructionLatencyPass(TAR));
  // Target-specific memory model (e.g. MSP430FR FRAM passes). Spliced in right
  // after the base latencies so their penalties accumulate into MBBLatencyMap.
  for (MachineFunctionPass *P : Target.getMemoryModelPasses(TAR))
    Passes.push_back(P);
  Passes.push_back(createMachineLoopBoundAgregatorPass(TAR));
  Passes.push_back(createFillMuGraphPass(TAR));
  Passes.push_back(createPathAnalysisPass(TAR));
  return Passes;
}

} // namespace llvm
