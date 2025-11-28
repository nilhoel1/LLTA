#ifndef LLTA_ADRESS_RESOLVER_H
#define LLTA_ADRESS_RESOLVER_H
#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class AdressResolverPass : public MachineFunctionPass {
public:
  static char ID;

  const bool DebugPrints = false;
  TimingAnalysisResults &TAR;
  AdressResolverPass(TimingAnalysisResults &TAR);

  // PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  bool runOnMachineBasicBlock(MachineBasicBlock &MBB);
  bool runOnMachineFunction(MachineFunction &F) override;
  bool doFinalization(Module &) override;
  bool doInitialization(Module &) override;

  void parseFile(std::string Filename);
  bool parseLine(std::string Line, int LineNumber);
  int lineHasLineNumber(std::string Line);
  bool isHex(std::string &In);
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  };

  virtual llvm::StringRef getPassName() const override {
    return "Adress Resolver Pass";
  }
};
} // namespace llvm,

namespace llvm {
MachineFunctionPass *createAdressResolverPass(TimingAnalysisResults &TAR);
} // namespace llvm
#endif
