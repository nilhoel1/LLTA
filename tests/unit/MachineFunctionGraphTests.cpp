//===- MachineFunctionGraphTests.cpp - end-to-end fillGraphWithFunction ---===//
//
// A dependency-light standalone test binary (no GoogleTest) that drives the two
// CFG-construction border cases through the REAL
// ProgramGraph::fillGraphWithFunction() path on a synthetic llvm::MachineFunction
// -- the cases that are unreachable through the C->ELF toolchain and that the
// ProgramGraphTests.cpp helper-level tests can only exercise on the extracted
// wireEntryExit() helper:
//
//   1. an EMPTY entry function (zero MachineBasicBlocks): previously read an
//      uninitialized "CurrentNode" when wiring the Exit edge; must now wire
//      Entry -> Exit directly without UB.
//   2. an entry function with blocks but NO return block (noreturn / infinite
//      loop): must fall back to wiring the last block -> Exit.
//
// A genuinely empty MachineFunction cannot be produced from C, so this is the
// only place case (1) is exercised through the production fillGraphWithFunction.
//
// The MachineFunction is built with a "Bogus" target (no real ISA), the standard
// LLVM unittest pattern from llvm/unittests/CodeGen/MFCommon.inc. The Bogus
// classes are copied here so the binary is self-contained. The MachineModuleInfo
// is kept alive for the MachineFunction's lifetime (the MF holds a reference to
// MMI's MCContext).
//
// Run via CTest (`ctest -R LLTAMachineFunctionGraphTests`) or `check-llta-cfg`.
//===----------------------------------------------------------------------===//

#include "Graph/ProgramGraph.h"
#include "Utility/DataMemoryAccess.h"

#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/PseudoSourceValueManager.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/TargetRegistry.h"

#include <iostream>
#include <memory>

using namespace llvm;

static int Checks = 0;
static int Failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++Checks;                                                                  \
    if (!(cond)) {                                                             \
      ++Failures;                                                              \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #cond   \
                << "\n";                                                       \
    }                                                                          \
  } while (0)

//===----------------------------------------------------------------------===//
// Bogus backend so we can create a MachineFunction without a real target.
// Copied from llvm/unittests/CodeGen/MFCommon.inc.
//===----------------------------------------------------------------------===//
namespace {

class BogusTargetLowering : public TargetLowering {
public:
  BogusTargetLowering(TargetMachine &TM) : TargetLowering(TM) {}
};

class BogusFrameLowering : public TargetFrameLowering {
public:
  BogusFrameLowering()
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 4) {}
  void emitPrologue(MachineFunction &, MachineBasicBlock &) const override {}
  void emitEpilogue(MachineFunction &, MachineBasicBlock &) const override {}

protected:
  bool hasFPImpl(const MachineFunction &) const override { return false; }
};

static TargetRegisterClass *const BogusRegisterClasses[] = {nullptr};

class BogusRegisterInfo : public TargetRegisterInfo {
public:
  BogusRegisterInfo()
      : TargetRegisterInfo(nullptr, BogusRegisterClasses, BogusRegisterClasses,
                           nullptr, nullptr, nullptr, LaneBitmask(~0u), nullptr,
                           nullptr) {
    InitMCRegisterInfo(nullptr, 0, 0, 0, nullptr, 0, nullptr, 0, nullptr,
                       nullptr, nullptr, nullptr, nullptr, 0, nullptr);
  }
  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *) const override {
    return nullptr;
  }
  ArrayRef<const uint32_t *> getRegMasks() const override { return {}; }
  ArrayRef<const char *> getRegMaskNames() const override { return {}; }
  BitVector getReservedRegs(const MachineFunction &) const override {
    return BitVector();
  }
  const RegClassWeight &
  getRegClassWeight(const TargetRegisterClass *) const override {
    static RegClassWeight Bogus{1, 16};
    return Bogus;
  }
  unsigned getRegUnitWeight(unsigned) const override { return 1; }
  unsigned getNumRegPressureSets() const override { return 0; }
  const char *getRegPressureSetName(unsigned) const override { return "bogus"; }
  unsigned getRegPressureSetLimit(const MachineFunction &,
                                  unsigned) const override {
    return 0;
  }
  const int *
  getRegClassPressureSets(const TargetRegisterClass *) const override {
    static const int Bogus[] = {0, -1};
    return &Bogus[0];
  }
  const int *getRegUnitPressureSets(unsigned) const override {
    static const int Bogus[] = {0, -1};
    return &Bogus[0];
  }
  Register getFrameRegister(const MachineFunction &) const override {
    return 0;
  }
  bool eliminateFrameIndex(MachineBasicBlock::iterator, int, unsigned,
                           RegScavenger * = nullptr) const override {
    return false;
  }
};

class BogusSubtarget : public TargetSubtargetInfo {
public:
  BogusSubtarget(TargetMachine &TM)
      : TargetSubtargetInfo(Triple(""), "", "", "", {}, {}, {}, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr),
        FL(), TL(TM) {}
  ~BogusSubtarget() override {}
  const TargetFrameLowering *getFrameLowering() const override { return &FL; }
  const TargetLowering *getTargetLowering() const override { return &TL; }
  const TargetInstrInfo *getInstrInfo() const override { return &TII; }
  const TargetRegisterInfo *getRegisterInfo() const override { return &TRI; }

private:
  BogusFrameLowering FL;
  BogusRegisterInfo TRI;
  BogusTargetLowering TL;
  TargetInstrInfo TII;
};

static TargetOptions getTargetOptionsForBogusMachine() {
  TargetOptions Opts;
  Opts.EmitCallSiteInfo = true;
  return Opts;
}

class BogusTargetMachine : public CodeGenTargetMachineImpl {
public:
  BogusTargetMachine()
      : CodeGenTargetMachineImpl(Target(), "", Triple(""), "", "",
                                 getTargetOptionsForBogusMachine(),
                                 Reloc::Static, CodeModel::Small,
                                 CodeGenOptLevel::Default),
        ST(*this) {}
  ~BogusTargetMachine() override {}
  const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return &ST;
  }

private:
  BogusSubtarget ST;
};

static BogusTargetMachine *createTargetMachine() {
  static BogusTargetMachine BogusTM;
  return &BogusTM;
}

// Owns an LLVMContext/Module/MachineModuleInfo and a MachineFunction, keeping
// MMI alive for the MF's lifetime (the MF holds a reference to MMI's MCContext).
struct MFFixture {
  LLVMContext Ctx;
  Module M;
  MachineModuleInfo MMI;
  Function *F;
  std::unique_ptr<MachineFunction> MF;

  MFFixture()
      : M("test", Ctx), MMI(createTargetMachine()) {
    auto *TM = createTargetMachine();
    auto *FT = FunctionType::get(Type::getVoidTy(Ctx), false);
    F = Function::Create(FT, GlobalValue::ExternalLinkage, "test", &M);
    MF = std::make_unique<MachineFunction>(*F, *TM, *TM->getSubtargetImpl(*F),
                                           MMI.getContext(), /*FunctionNum=*/42);
  }

  // Append a fresh (empty, hence non-return) MachineBasicBlock.
  MachineBasicBlock *addBlock() {
    auto *MBB = MF->CreateMachineBasicBlock();
    MF->insert(MF->end(), MBB);
    return MBB;
  }

  // A bare MachineInstr carrying the given MCID flags (e.g. MayLoad). The desc
  // must outlive the instruction (MachineInstr stores a pointer to it), so the
  // caller passes a long-lived MCInstrDesc.
  MachineInstr *makeInstr(const MCInstrDesc &Desc) {
    return MF->CreateMachineInstr(Desc, DebugLoc());
  }

  // A provably-stack (SRAM) load memory operand.
  MachineMemOperand *stackMMO() {
    return MF->getMachineMemOperand(
        MachinePointerInfo(MF->getPSVManager().getStack()),
        MachineMemOperand::MOLoad, /*Size=*/2, Align(2));
  }

  // A memory operand at an unknown address (no PseudoSourceValue, no IR value):
  // cannot be proven non-FRAM.
  MachineMemOperand *unknownMMO() {
    return MF->getMachineMemOperand(MachinePointerInfo(),
                                    MachineMemOperand::MOLoad, /*Size=*/2,
                                    Align(2));
  }
};

} // namespace

// Case 1: empty entry MachineFunction -> Entry wired directly to Exit, no UB.
static void testEmptyMachineFunction() {
  MFFixture Fx;
  CHECK(Fx.MF->empty()); // zero MachineBasicBlocks

  ProgramGraph G;
  G.fillGraphWithFunction(*Fx.MF, /*IsEntry=*/true, /*MBBLatencyMap=*/{},
                          /*LoopBoundMap=*/{}, /*MLI=*/nullptr,
                          /*IrreducibleBackEdges=*/{});

  CHECK(G.HasEntryNode);
  // Only the synthetic Entry and Exit nodes exist.
  CHECK(G.getNodes().size() == 2);
  // Entry -> Exit, and Exit is a sink.
  const auto &EntrySucc = G.getSuccessors(G.EntryNodeId);
  CHECK(EntrySucc.size() == 1);
  if (EntrySucc.size() == 1) {
    unsigned Exit = *EntrySucc.begin();
    CHECK(Exit != G.EntryNodeId);
    CHECK(G.getSuccessors(Exit).empty());
  }
}

// Case 2: entry function with blocks but no return block -> fallback wires the
// last block to Exit, and the Entry/CFG edges are present.
static void testNoReturnBlockMachineFunction() {
  MFFixture Fx;
  auto *MBB0 = Fx.addBlock();
  auto *MBB1 = Fx.addBlock();
  MBB0->addSuccessor(MBB1); // chain: entry-block -> second-block
  CHECK(!MBB0->isReturnBlock());
  CHECK(!MBB1->isReturnBlock());

  ProgramGraph G;
  G.fillGraphWithFunction(*Fx.MF, /*IsEntry=*/true, /*MBBLatencyMap=*/{},
                          /*LoopBoundMap=*/{}, /*MLI=*/nullptr,
                          /*IrreducibleBackEdges=*/{});

  CHECK(G.HasEntryNode);
  unsigned N0 = G.MBBToNodeMap.at(MBB0);
  unsigned N1 = G.MBBToNodeMap.at(MBB1);
  CHECK(G.hasEdge(G.EntryNodeId, N0)); // Entry -> first block
  CHECK(G.hasEdge(N0, N1));            // CFG successor edge preserved
  // Fallback: last block -> Exit (no return block existed).
  const auto &LastSucc = G.getSuccessors(N1);
  CHECK(LastSucc.size() == 1);
  if (LastSucc.size() == 1) {
    unsigned Exit = *LastSucc.begin();
    CHECK(Exit != G.EntryNodeId);
    CHECK(G.getSuccessors(Exit).empty()); // Exit is a sink
  }
}

// framDataAccessWords: the soundness classifier shared by the FRAM cache and
// no-cache passes. Provably-stack accesses are free; anything not proven
// non-wait-state (unknown address, or no memoperand info) is assumed FRAM and
// charged; a non-memory instruction is free.
static void testFramDataAccessWords() {
  MFFixture Fx;

  // Long-lived descs (MachineInstr keeps a pointer). Field 10 is the flag word.
  const MCInstrDesc LoadDesc = {
      TargetOpcode::COPY, 0, 0, 0, 0, 0, 0, 0, 0, (1ULL << MCID::MayLoad), 0};
  const MCInstrDesc RmwDesc = {
      TargetOpcode::COPY, 0,
      0,                  0,
      0,                  0,
      0,                  0,
      0,                  (1ULL << MCID::MayLoad) | (1ULL << MCID::MayStore),
      0};
  const MCInstrDesc PlainDesc = {TargetOpcode::COPY, 0, 0, 0, 0, 0,
                                 0,                  0, 0, 0, 0};

  // mayLoad + provably-stack operand -> not charged.
  MachineInstr *Stack = Fx.makeInstr(LoadDesc);
  Stack->addMemOperand(*Fx.MF, Fx.stackMMO());
  CHECK(framDataAccessWords(*Stack) == 0u);

  // mayLoad + unprovable (unknown) operand -> assume FRAM -> charged once.
  MachineInstr *Unknown = Fx.makeInstr(LoadDesc);
  Unknown->addMemOperand(*Fx.MF, Fx.unknownMMO());
  CHECK(framDataAccessWords(*Unknown) == 1u);

  // mayLoad + NO memoperand info -> unknown address -> charged.
  MachineInstr *NoInfo = Fx.makeInstr(LoadDesc);
  CHECK(framDataAccessWords(*NoInfo) == 1u);

  // read-modify-write with no memoperand info -> load + store both charged.
  MachineInstr *Rmw = Fx.makeInstr(RmwDesc);
  CHECK(framDataAccessWords(*Rmw) == 2u);

  // No memory access -> free.
  MachineInstr *Plain = Fx.makeInstr(PlainDesc);
  CHECK(framDataAccessWords(*Plain) == 0u);
}

int main() {
  testEmptyMachineFunction();
  testNoReturnBlockMachineFunction();
  testFramDataAccessWords();

  if (Failures == 0) {
    std::cout << "All " << Checks << " checks passed.\n";
    return 0;
  }
  std::cerr << Failures << " of " << Checks << " checks FAILED.\n";
  return 1;
}
