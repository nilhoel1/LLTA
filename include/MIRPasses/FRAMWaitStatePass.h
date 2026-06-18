#ifndef LLVM_LLTA_MIRPASSES_FRAMWAITSTATEPASS_H
#define LLVM_LLTA_MIRPASSES_FRAMWAITSTATEPASS_H

#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

/**
 * FRAMWaitStatePass — MSP430 FRAM instruction-fetch wait-state model.
 *
 * On MSP430FR devices the program lives in FRAM, which cannot be accessed at
 * full speed above 8 MHz: each FRAM access then costs additional MCLK wait
 * states (configured via the FRCTL NACCESS field). The base instruction
 * latencies in InstructionLatencyPass assume zero wait states, so this pass
 * adds the missing cycles.
 *
 * For every code-emitting MachineInstr whose resolved address (from
 * AdressResolverPass, stored in TimingAnalysisResults::InstructionAddressMap)
 * lies in the FRAM region (address >= FRAMStart), it adds
 *   FRAMWaitStates * (number of 16-bit code words fetched)
 * cycles to the instruction's basic block in MBBLatencyMap. The word count is
 * derived from the gap to the next resolved instruction address.
 *
 * The model is a sound WCET over-approximation: it assumes every FRAM fetch
 * pays the wait state and ignores the FR5xx FRAM cache.
 *
 * The pass must run after InstructionLatencyPass (which populates
 * MBBLatencyMap) and after AdressResolverPass (which populates the addresses
 * and FRAMStart). It is a no-op unless -fram-wait-states > 0 and -fram-start
 * was supplied, so default runs are unaffected.
 */
class FRAMWaitStatePass : public MachineFunctionPass {
public:
  static char ID;
  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;

  FRAMWaitStatePass(TimingAnalysisResults &TAR);

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  bool runOnMachineFunction(MachineFunction &F) override;

  virtual llvm::StringRef getPassName() const override {
    return "MSP430 FRAM Wait-State Pass";
  }
};

MachineFunctionPass *createFRAMWaitStatePass(TimingAnalysisResults &TAR);
} // namespace llvm

#endif // LLVM_LLTA_MIRPASSES_FRAMWAITSTATEPASS_H
