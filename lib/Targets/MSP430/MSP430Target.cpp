#include "Targets/MSP430/MSP430Target.h"

#include "MCTargetDesc/MSP430MCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace llta {

// TODO I dont know how the latencies will be represented if the FRAM Controller
// is analysed. Maybe use a struct instead of a simple unsigned int.

// FIXME Currently we assume CPUx on the MSP430, this should be corrected, when
// llvm also supports the MSP430 CPUX. Issue with this is latencies only hold
// for the upper 64kb of memory on MSP430 CPUx
//
// Soundness note (lower 64 KB / FR5994 FRAM): these are the SLAU445I 0-wait
// architectural CPU-cycle counts. The "upper 64 kb" caveat is about CPUX
// *extended* (20-bit) addressing of memory above 64 KB, which only *adds*
// cycles; in the lower 64 KB FRAM (16-bit addressing, where our code runs) the
// basic-form counts apply and are an upper bound on the true CPU cycles. All
// FRAM memory-access overhead is charged separately and additively by the FRAM
// fetch line-fill (-fram-line-fill-cycles) and data-access (-fram-wait-states)
// penalties, so the base table stays a sound upper bound here.
unsigned MSP430Target::getInstructionLatency(const MachineInstr &I) const {
  return getMSP430Latency(I);
}

std::optional<unsigned>
MSP430Target::getInstructionLatency(const MCInst &I) const {
  return getMSP430Latency(I);
}

// True for `add rX,rX` / `addc rX,rX`, the backend's emulation of a left
// shift / rotate-left by one (both source operands the same register). A plain
// `add rA,rB` is a real addition, not a shift.
static bool isRegSelfAddShift(const MachineInstr &MI) {
  return MI.getNumOperands() >= 3 && MI.getOperand(1).isReg() &&
         MI.getOperand(2).isReg() &&
         MI.getOperand(1).getReg() == MI.getOperand(2).getReg();
}

std::optional<unsigned>
MSP430Target::getImplicitLoopBound(const MachineLoop &L) const {
  // The backend emits a multi-bit shift as a single block that branches to
  // itself; reject anything more complex.
  if (L.getNumBlocks() != 1)
    return std::nullopt;
  const MachineBasicBlock *MBB = L.getHeader();
  if (!MBB->isSuccessor(MBB))
    return std::nullopt;

  // Bit width of the value being shifted, summed over the shift/rotate ops that
  // make up the (possibly multi-word) shift -- e.g. a 32-bit shift is `rra`
  // (high word) + `rrc` (low word) => 32. The C shift amount is strictly below
  // this width, so the width is a sound upper bound on the trip count. A loop
  // that mixes two independent shifts only over-approximates, which stays
  // sound.
  unsigned ShiftedWidthBits = 0;
  bool SawCounterDecrement = false;

  for (const MachineInstr &MI : *MBB) {
    switch (MI.getOpcode()) {
    // Right shift / rotate-right-through-carry (register form).
    case MSP430::RRA16r:
    case MSP430::RRC16r:
      ShiftedWidthBits += 16;
      break;
    case MSP430::RRA8r:
    case MSP430::RRC8r:
      ShiftedWidthBits += 8;
      break;
    // Left shift / rotate-left, emulated as add/addc of a register with itself.
    case MSP430::ADD16rr:
    case MSP430::ADDC16rr:
      if (!isRegSelfAddShift(MI))
        return std::nullopt; // a real addition => not a shift loop
      ShiftedWidthBits += 16;
      break;
    case MSP430::ADD8rr:
    case MSP430::ADDC8rr:
      if (!isRegSelfAddShift(MI))
        return std::nullopt;
      ShiftedWidthBits += 8;
      break;
    // The loop-counter decrement (`sub[.b] #1, rN`, also via subc/dec), which
    // sets the flags the latch branch tests.
    case MSP430::SUB16ri:
    case MSP430::SUB8ri:
    case MSP430::SUBC16ri:
    case MSP430::SUBC8ri:
      SawCounterDecrement = true;
      break;
    // Carry setup for a logical shift (`clrc` == `bic #1, sr`), the latch
    // branch (JCC), an unconditional exit branch (`jmp`/`br #label`), and an
    // optional explicit counter compare -- allowed, no width.
    case MSP430::BIC16rc:
    case MSP430::BIC8rc:
    case MSP430::JCC:
    case MSP430::JMP:
    case MSP430::Bi: // long unconditional branch `br #label` (loop exit)
    case MSP430::CMP16ri:
    case MSP430::CMP8ri:
      break;
    // Book-keeping with no run-time effect.
    case MSP430::CFI_INSTRUCTION:
    case TargetOpcode::DBG_VALUE:
    case TargetOpcode::DBG_LABEL:
    case TargetOpcode::DBG_INSTR_REF:
    case TargetOpcode::DBG_PHI:
    case TargetOpcode::DBG_VALUE_LIST:
      break;
    // A memory access, call, or any other ALU op means this is not a pure shift
    // loop. Stay conservative and decline to bound it.
    default:
      return std::nullopt;
    }
  }

  // Require both a real shift and a counter, else the idiom did not match.
  if (ShiftedWidthBits == 0 || !SawCounterDecrement)
    return std::nullopt;
  return ShiftedWidthBits;
}

// Core latency table, keyed only on the opcode and whether any operand
// references PC (the only operand property the table needs). Keeping it free of
// the MachineInstr/MCInst operand APIs lets it serve both the codegen MIR path
// and the disassembled-ELF (library-call costing) path. Returns nullopt for an
// opcode with no latency model; callers decide how to diagnose that.
static std::optional<unsigned> msp430LatencyForOpcode(unsigned Opcode,
                                                      bool HasPCOperand) {
  // r = RN, RM
  // m = x(Rn), x(Rm), EDE, &EDE
  // n = @Rn
  // p = @Rn+
  // c, i = #N
  // if two are used first is destination secnd is source
  // E.g.: ADD16rm -> r = Destination, m = Source
  switch (Opcode) {
  // Format-III Instructions

  // return from subroutine
  case MSP430::RET:
    return 4;
  // return from interrupt
  case MSP430::RETI:
    return 5;

  // All jump instructions require one code word and take two CPU cycles to
  // execute, regardless of whether the jump is taken or not.
  case MSP430::JCC: // conditional
  case MSP430::JMP:
    return 2;
  // END Format-III Instructions

  // Format-II Instructions [SLAU445I p.154]
  case MSP430::RRA16m:
  case MSP430::RRA8m:
  case MSP430::RRC16m:
  case MSP430::RRC8m:
  case MSP430::SWPB16m:
  case MSP430::SEXT16m:
    return 4;

  case MSP430::RRA16n:
  case MSP430::RRA8n:
  case MSP430::RRC16n:
  case MSP430::RRC8n:
  case MSP430::SWPB16n:
  case MSP430::SEXT16n:
    return 3;

  case MSP430::RRA16p:
  case MSP430::RRA8p:
  case MSP430::RRC16p:
  case MSP430::RRC8p:
  case MSP430::SWPB16p:
  case MSP430::SEXT16p:
    return 3;

  case MSP430::RRA16r:
  case MSP430::RRA8r:
  case MSP430::RRC16r:
  case MSP430::RRC8r:
  case MSP430::SWPB16r:
  case MSP430::SEXT16r:
  case MSP430::ZEXT16r:
    return 1;

  case MSP430::CALLm:
    return 5; // FIXME &EDE is 6

  case MSP430::CALLi: // 5 on Non MSP430X
  case MSP430::CALLn:
  case MSP430::CALLp: // 5 on Non MSP430x
  case MSP430::CALLr:
    return 4;
  case MSP430::POP16r:
  case MSP430::PUSH16c: // 4 on Non MSP430X
  case MSP430::PUSH16i: // 4 on Non MSP430X
  case MSP430::PUSH16r:
  case MSP430::PUSH8r:
    return 3;
  // End Format-II Instructions

  // Format-I Instructions
  // mc and mi translates to #N and x(RM), EDE, &EDE [SLAU445I p.155]
  case MSP430::ADD16mc:
  case MSP430::ADD16mi:
  case MSP430::ADD8mc:
  case MSP430::ADD8mi:
  case MSP430::ADDC16mc:
  case MSP430::ADDC16mi:
  case MSP430::ADDC8mc:
  case MSP430::ADDC8mi:
  case MSP430::AND16mc:
  case MSP430::AND16mi:
  case MSP430::AND8mc:
  case MSP430::AND8mi:
  case MSP430::BIC16mc:
  case MSP430::BIC16mi:
  case MSP430::BIC8mc:
  case MSP430::BIC8mi:
  case MSP430::BIS16mc:
  case MSP430::BIS16mi:
  case MSP430::BIS8mc:
  case MSP430::BIS8mi:
  case MSP430::DADD16mc: // Emulated
  case MSP430::DADD16mi: // Emulated
  case MSP430::DADD8mc:  // Emulated
  case MSP430::DADD8mi:  // Emulated
  case MSP430::SUB16mc:
  case MSP430::SUB8mc:
  case MSP430::SUBC16mc:
  case MSP430::SUBC16mi:
  case MSP430::SUBC8mc:
  case MSP430::SUBC8mi:
  case MSP430::XOR16mc:
  case MSP430::XOR16mi:
  case MSP430::XOR8mc:
  case MSP430::XOR8mi:
    return 5;
  case MSP430::CMP16mc:
  case MSP430::CMP16mi:
  case MSP430::CMP8mc:
  case MSP430::CMP8mi:
  case MSP430::BIT16mc:
  case MSP430::BIT16mi:
  case MSP430::BIT8mc:
  case MSP430::BIT8mi:
  case MSP430::MOV16mc:
  case MSP430::MOV16mi:
  case MSP430::MOV8mc:
  case MSP430::MOV8mi:
    return 4; // MOV, BIT and CMP Instructions execute in one fewer cycle
              // [SLAU445I p.155]

  // mm translates to x(Rn), EDE, &EDE and x(RM), EDE, &EDE [SLAU445I p.155]
  case MSP430::ADD16mm:
  case MSP430::ADD8mm:
  case MSP430::ADDC16mm:
  case MSP430::ADDC8mm:
  case MSP430::AND16mm:
  case MSP430::AND8mm:
  case MSP430::BIC16mm:
  case MSP430::BIC8mm:
  case MSP430::BIS16mm:
  case MSP430::BIS8mm:
  case MSP430::DADD16mm: // Emulated
  case MSP430::DADD8mm:  // Emulated
  case MSP430::SUB16mm:
  case MSP430::SUB8mm:
  case MSP430::SUBC16mm:
  case MSP430::SUBC8mm:
  case MSP430::XOR16mm:
  case MSP430::XOR8mm:
    return 6;
  case MSP430::BIT16mm:
  case MSP430::BIT8mm:
  case MSP430::CMP16mm:
  case MSP430::CMP8mm:
  case MSP430::MOV16mm:
  case MSP430::MOV8mm:
    return 5; // MOV, BIT and CMP Instructions execute in one fewer cycle
              // [SLAU445I p.155]

  // mn translates to @Rn and x(RM), EDE, &EDE [SLAU445I p.155]
  case MSP430::ADD16mn:
  case MSP430::ADD8mn:
  case MSP430::ADDC16mn:
  case MSP430::ADDC8mn:
  case MSP430::AND16mn:
  case MSP430::AND8mn:
  case MSP430::BIC16mn:
  case MSP430::BIC8mn:
  case MSP430::BIS16mn:
  case MSP430::BIS8mn:
  case MSP430::DADD16mn: // Emulated
  case MSP430::DADD8mn:  // Emulated
  case MSP430::SUB16mn:
  case MSP430::SUB8mn:
  case MSP430::SUBC16mn:
  case MSP430::SUBC8mn:
  case MSP430::XOR16mn:
  case MSP430::XOR8mn:
    return 5;
  case MSP430::BIT16mn:
  case MSP430::BIT8mn:
  case MSP430::CMP16mn:
  case MSP430::CMP8mn:
  case MSP430::MOV16mn:
  case MSP430::MOV8mn:
    return 4; // MOV, BIT and CMP Instructions execute in one fewer cycle
              // [SLAU445I p.155]

  // mp translates to @Rn+ and x(RM), EDE, &EDE [SLAU445I p.155]
  case MSP430::ADD16mp:
  case MSP430::ADD8mp:
  case MSP430::ADDC16mp:
  case MSP430::ADDC8mp:
  case MSP430::AND16mp:
  case MSP430::AND8mp:
  case MSP430::BIC16mp:
  case MSP430::BIC8mp:
  case MSP430::BIS16mp:
  case MSP430::BIS8mp:
  case MSP430::DADD16mp: // Emulated
  case MSP430::DADD8mp:  // Emulated
  case MSP430::SUB16mp:
  case MSP430::SUB8mp:
  case MSP430::SUBC16mp:
  case MSP430::SUBC8mp:
  case MSP430::XOR16mp:
  case MSP430::XOR8mp:
    return 5;
  case MSP430::BIT16mp:
  case MSP430::BIT8mp:
  case MSP430::CMP16mp:
  case MSP430::CMP8mp:
    return 4; // MOV, BIT and CMP Instructions execute in one fewer cycle
              // [SLAU445I p.155]

  // mr translates to Rn and x(RM), EDE, &EDE [SLAU445I p.155]
  case MSP430::ADD16mr:
  case MSP430::ADD8mr:
  case MSP430::ADDC16mr:
  case MSP430::ADDC8mr:
  case MSP430::AND16mr:
  case MSP430::AND8mr:
  case MSP430::BIC16mr:
  case MSP430::BIC8mr:
  case MSP430::BIS16mr:
  case MSP430::BIS8mr:
  case MSP430::DADD16mr: // Emulated
  case MSP430::DADD8mr:  // Emulated
  case MSP430::SUB16mr:
  case MSP430::SUB8mr:
  case MSP430::SUBC16mr:
  case MSP430::SUBC8mr:
  case MSP430::XOR16mr:
  case MSP430::XOR8mr:
    return 4;
  case MSP430::MOV16mr:
  case MSP430::MOV8mr:
  case MSP430::BIT16mr:
  case MSP430::BIT8mr:
  case MSP430::CMP16mr:
  case MSP430::CMP8mr:
    return 3; // MOV, BIT and CMP Instructions execute in one fewer cycle
              // [SLAU445I p.155]

  // rc and ri translates to #N and Rm, PC [SLAU445I p.155]
  case MSP430::ADD16rc:
  case MSP430::ADD16ri:
  case MSP430::ADD8rc:
  case MSP430::ADD8ri:
  case MSP430::ADDC16rc:
  case MSP430::ADDC16ri:
  case MSP430::ADDC8rc:
  case MSP430::ADDC8ri:
  case MSP430::AND16rc:
  case MSP430::AND16ri:
  case MSP430::AND8rc:
  case MSP430::AND8ri:
  case MSP430::BIC16rc:
  case MSP430::BIC16ri:
  case MSP430::BIC8rc:
  case MSP430::BIC8ri:
  case MSP430::BIS16rc:
  case MSP430::BIS16ri:
  case MSP430::BIS8rc:
  case MSP430::BIS8ri:
  case MSP430::BIT16rc:
  case MSP430::BIT16ri:
  case MSP430::BIT8rc:
  case MSP430::BIT8ri:
  case MSP430::CMP16rc:
  case MSP430::CMP16ri:
  case MSP430::CMP8rc:
  case MSP430::CMP8ri:
  case MSP430::DADD16rc: // Emulated
  case MSP430::DADD16ri: // Emulated
  case MSP430::DADD8rc:  // Emulated
  case MSP430::DADD8ri:  // Emulated
  case MSP430::SUB16rc:
  case MSP430::SUB16ri:
  case MSP430::SUB8rc:
  case MSP430::SUB8ri:
  case MSP430::SUBC16rc:
  case MSP430::SUBC16ri:
  case MSP430::SUBC8rc:
  case MSP430::SUBC8ri:
  case MSP430::XOR16rc:
  case MSP430::XOR16ri:
  case MSP430::XOR8rc:
  case MSP430::XOR8ri:
  case MSP430::MOV16rc:
  case MSP430::MOV16ri:
  case MSP430::MOV8rc:
  case MSP430::MOV8ri:
  case MSP430::Bi: // Emulated
    return HasPCOperand ? 3 : 2;

  // rm translates to x(Rn) and Rm, PC [SLAU445I p.155]
  case MSP430::ADD16rm:
  case MSP430::ADD8rm:
  case MSP430::ADDC16rm:
  case MSP430::ADDC8rm:
  case MSP430::AND16rm:
  case MSP430::AND8rm:
  case MSP430::BIC16rm:
  case MSP430::BIC8rm:
  case MSP430::BIS16rm:
  case MSP430::BIS8rm:
  case MSP430::BIT16rm:
  case MSP430::BIT8rm:
  case MSP430::CMP16rm:
  case MSP430::CMP8rm:
  case MSP430::DADD16rm: // Emulated
  case MSP430::DADD8rm:  // Emulated
  case MSP430::SUB16rm:
  case MSP430::SUB8rm:
  case MSP430::SUBC16rm:
  case MSP430::SUBC8rm:
  case MSP430::XOR16rm:
  case MSP430::XOR8rm:
  case MSP430::MOV16rm:
  case MSP430::MOV8rm:
  case MSP430::MOVZX16rm8:
  case MSP430::Bm: // Emulated
    return 3;      // 5 on Non MSP430 non X (independent of PC operand)

  // rn translates to @Rn and Rm, PC [SLAU445I p.155]
  case MSP430::ADD16rn:
  case MSP430::ADD8rn:
  case MSP430::ADDC16rn:
  case MSP430::ADDC8rn:
  case MSP430::AND16rn:
  case MSP430::AND8rn:
  case MSP430::BIC16rn:
  case MSP430::BIC8rn:
  case MSP430::BIS16rn:
  case MSP430::BIS8rn:
  case MSP430::BIT16rn:
  case MSP430::BIT8rn:
  case MSP430::CMP16rn:
  case MSP430::CMP8rn:
  case MSP430::DADD16rn: // Emulated
  case MSP430::DADD8rn:  // Emulated
  case MSP430::SUB16rn:
  case MSP430::SUB8rn:
  case MSP430::SUBC16rn:
  case MSP430::SUBC8rn:
  case MSP430::XOR16rn:
  case MSP430::XOR8rn:
  case MSP430::MOV16rn:
  case MSP430::MOV8rn:
    return 2; // 4 on Non MSP430 non X (independent of PC operand)

  // rp translates to @Rn+ and Rm, PC [SLAU445I p.155]
  case MSP430::ADD16rp:
  case MSP430::ADD8rp:
  case MSP430::ADDC16rp:
  case MSP430::ADDC8rp:
  case MSP430::AND16rp:
  case MSP430::AND8rp:
  case MSP430::BIC16rp:
  case MSP430::BIC8rp:
  case MSP430::BIS16rp:
  case MSP430::BIS8rp:
  case MSP430::BIT16rp:
  case MSP430::BIT8rp:
  case MSP430::CMP16rp:
  case MSP430::CMP8rp:
  case MSP430::DADD16rp: // Emulated
  case MSP430::DADD8rp:  // Emulated
  case MSP430::SUB16rp:
  case MSP430::SUB8rp:
  case MSP430::SUBC16rp:
  case MSP430::SUBC8rp:
  case MSP430::XOR16rp:
  case MSP430::XOR8rp:
  case MSP430::MOV16rp:
  case MSP430::MOV8rp:
    return HasPCOperand ? 3 : 2; // 4 on Non MSP430 non X

  // rr translates to Rn and Rm or Rn and PC [SLAU445I p.155]
  case MSP430::ADD16rr:
  case MSP430::ADD8rr:
  case MSP430::ADDC16rr:
  case MSP430::ADDC8rr:
  case MSP430::AND16rr:
  case MSP430::AND8rr:
  case MSP430::BIC16rr:
  case MSP430::BIC8rr:
  case MSP430::BIS16rr:
  case MSP430::BIS8rr:
  case MSP430::BIT16rr:
  case MSP430::BIT8rr:
  case MSP430::CMP16rr:
  case MSP430::CMP8rr:
  case MSP430::DADD16rr: // Emulated
  case MSP430::DADD8rr:  // Emulated
  case MSP430::SUB16rr:
  case MSP430::SUB8rr:
  case MSP430::SUBC16rr:
  case MSP430::SUBC8rr:
  case MSP430::XOR16rr:
  case MSP430::XOR8rr:
  case MSP430::MOV16rr:
  case MSP430::MOV8rr:
  case MSP430::MOVZX16rr8:
  case MSP430::Br:               // Emulated
    return HasPCOperand ? 2 : 1; // 3 on Non MSP430 non X
    // End Format-I Instructions

  case MSP430::CFI_INSTRUCTION:
    return 0;

  // Debug Instructions should not ahve any Latencies
  case TargetOpcode::DBG_VALUE:
  case TargetOpcode::DBG_LABEL:
  case TargetOpcode::DBG_INSTR_REF:
  case TargetOpcode::DBG_PHI:
  case TargetOpcode::DBG_VALUE_LIST:
    return 0;

  // Inline ASM
  case TargetOpcode::INLINEASM:
    return 1; // TODO We just assume 1x nop all the time

  default:
    return std::nullopt;
  }
}

/// True if any operand of \p I is the PC register. The latency table needs only
/// this one operand property (see msp430LatencyForOpcode).
static bool anyOperandIsPC(const MachineInstr &I) {
  for (const MachineOperand &MO : I.operands())
    if (MO.isReg() && MO.getReg() == MSP430::PC)
      return true;
  return false;
}

static bool anyOperandIsPC(const MCInst &I) {
  for (unsigned Idx = 0, E = I.getNumOperands(); Idx < E; ++Idx) {
    const MCOperand &MO = I.getOperand(Idx);
    if (MO.isReg() && MO.getReg() == MSP430::PC)
      return true;
  }
  return false;
}

unsigned getMSP430Latency(const MachineInstr &I) {
  if (std::optional<unsigned> L =
          msp430LatencyForOpcode(I.getOpcode(), anyOperandIsPC(I)))
    return *L;
  errs() << "No Latency assigned to Inst: " << I << "\n";
  assert(0 && "Instruction has no Latency!");
  return 0;
}

std::optional<unsigned> getMSP430Latency(const MCInst &I) {
  return msp430LatencyForOpcode(I.getOpcode(), anyOperandIsPC(I));
}

void MSP430Target::checkInstruction(const MachineInstr &I) const {
  // Latecy Information found here:
  switch (I.getOpcode()) {
  case MSP430::ADD16mc: //
  case MSP430::ADD16mi:
  case MSP430::ADD16mm:
  case MSP430::ADD16mn:
  case MSP430::ADD16mp:
  case MSP430::ADD16mr: // , mem
  case MSP430::ADD16rc: // cg16imm, GR
  case MSP430::ADD16ri: // imm, GR
  case MSP430::ADD16rm: // GR, mem [ADDR]
  case MSP430::ADD16rn: // GR, indreg(imm16)
  case MSP430::ADD16rp: // GR, Pointer location, mayload
  case MSP430::ADD16rr: // GR, GR
  case MSP430::ADD8mc:
  case MSP430::ADD8mi:
  case MSP430::ADD8mm:
  case MSP430::ADD8mn:
  case MSP430::ADD8mp:
  case MSP430::ADD8mr:
  case MSP430::ADD8rc:
  case MSP430::ADD8ri:
  case MSP430::ADD8rm:
  case MSP430::ADD8rn:
  case MSP430::ADD8rp:
  case MSP430::ADD8rr:
  case MSP430::ADDC16mc:
  case MSP430::ADDC16mi:
  case MSP430::ADDC16mm:
  case MSP430::ADDC16mn:
  case MSP430::ADDC16mp:
  case MSP430::ADDC16mr:
  case MSP430::ADDC16rc:
  case MSP430::ADDC16ri:
  case MSP430::ADDC16rm:
  case MSP430::ADDC16rn:
  case MSP430::ADDC16rp:
  case MSP430::ADDC16rr:
  case MSP430::ADDC8mc:
  case MSP430::ADDC8mi:
  case MSP430::ADDC8mm:
  case MSP430::ADDC8mn:
  case MSP430::ADDC8mp:
  case MSP430::ADDC8mr:
  case MSP430::ADDC8rc:
  case MSP430::ADDC8ri:
  case MSP430::ADDC8rm:
  case MSP430::ADDC8rn:
  case MSP430::ADDC8rp:
  case MSP430::ADDC8rr:
  case MSP430::AND16mc:
  case MSP430::AND16mi:
  case MSP430::AND16mm:
  case MSP430::AND16mn:
  case MSP430::AND16mp:
  case MSP430::AND16mr:
  case MSP430::AND16rc:
  case MSP430::AND16ri:
  case MSP430::AND16rm:
  case MSP430::AND16rn:
  case MSP430::AND16rp:
  case MSP430::AND16rr:
  case MSP430::AND8mc:
  case MSP430::AND8mi:
  case MSP430::AND8mm:
  case MSP430::AND8mn:
  case MSP430::AND8mp:
  case MSP430::AND8mr:
  case MSP430::AND8rc:
  case MSP430::AND8ri:
  case MSP430::AND8rm:
  case MSP430::AND8rn:
  case MSP430::AND8rp:
  case MSP430::AND8rr:
  case MSP430::BIC16mc:
  case MSP430::BIC16mi:
  case MSP430::BIC16mm:
  case MSP430::BIC16mn:
  case MSP430::BIC16mp:
  case MSP430::BIC16mr:
  case MSP430::BIC16rc:
  case MSP430::BIC16ri:
  case MSP430::BIC16rm:
  case MSP430::BIC16rn:
  case MSP430::BIC16rp:
  case MSP430::BIC16rr:
  case MSP430::BIC8mc:
  case MSP430::BIC8mi:
  case MSP430::BIC8mm:
  case MSP430::BIC8mn:
  case MSP430::BIC8mp:
  case MSP430::BIC8mr:
  case MSP430::BIC8rc:
  case MSP430::BIC8ri:
  case MSP430::BIC8rm:
  case MSP430::BIC8rn:
  case MSP430::BIC8rp:
  case MSP430::BIC8rr:
  case MSP430::BIS16mc:
  case MSP430::BIS16mi:
  case MSP430::BIS16mm:
  case MSP430::BIS16mn:
  case MSP430::BIS16mp:
  case MSP430::BIS16mr:
  case MSP430::BIS16rc:
  case MSP430::BIS16ri:
  case MSP430::BIS16rm:
  case MSP430::BIS16rn:
  case MSP430::BIS16rp:
  case MSP430::BIS16rr:
  case MSP430::BIS8mc:
  case MSP430::BIS8mi:
  case MSP430::BIS8mm:
  case MSP430::BIS8mn:
  case MSP430::BIS8mp:
  case MSP430::BIS8mr:
  case MSP430::BIS8rc:
  case MSP430::BIS8ri:
  case MSP430::BIS8rm:
  case MSP430::BIS8rn:
  case MSP430::BIS8rp:
  case MSP430::BIS8rr:
  case MSP430::BIT16mc:
  case MSP430::BIT16mi:
  case MSP430::BIT16mm:
  case MSP430::BIT16mn:
  case MSP430::BIT16mp:
  case MSP430::BIT16mr:
  case MSP430::BIT16rc:
  case MSP430::BIT16ri:
  case MSP430::BIT16rm:
  case MSP430::BIT16rn:
  case MSP430::BIT16rp:
  case MSP430::BIT16rr:
  case MSP430::BIT8mc:
  case MSP430::BIT8mi:
  case MSP430::BIT8mm:
  case MSP430::BIT8mn:
  case MSP430::BIT8mp:
  case MSP430::BIT8mr:
  case MSP430::BIT8rc:
  case MSP430::BIT8ri:
  case MSP430::BIT8rm:
  case MSP430::BIT8rn:
  case MSP430::BIT8rp:
  case MSP430::BIT8rr:
  case MSP430::CMP16mc:
  case MSP430::CMP16mi:
  case MSP430::CMP16mm:
  case MSP430::CMP16mn:
  case MSP430::CMP16mp:
  case MSP430::CMP16mr:
  case MSP430::CMP16rc:
  case MSP430::CMP16ri:
  case MSP430::CMP16rm:
  case MSP430::CMP16rn:
  case MSP430::CMP16rp:
  case MSP430::CMP16rr:
  case MSP430::CMP8mc:
  case MSP430::CMP8mi:
  case MSP430::CMP8mm:
  case MSP430::CMP8mn:
  case MSP430::CMP8mp:
  case MSP430::CMP8mr:
  case MSP430::CMP8rc:
  case MSP430::CMP8ri:
  case MSP430::CMP8rm:
  case MSP430::CMP8rn:
  case MSP430::CMP8rp:
  case MSP430::CMP8rr:
  // The DADD instruction needs 1 extra cycle.
  case MSP430::DADD16mc:
  case MSP430::DADD16mi:
  case MSP430::DADD16mm:
  case MSP430::DADD16mn:
  case MSP430::DADD16mp:
  case MSP430::DADD16mr:
  case MSP430::DADD16rc:
  case MSP430::DADD16ri:
  case MSP430::DADD16rm:
  case MSP430::DADD16rn:
  case MSP430::DADD16rp:
  case MSP430::DADD16rr:
  case MSP430::DADD8mc:
  case MSP430::DADD8mi:
  case MSP430::DADD8mm:
  case MSP430::DADD8mn:
  case MSP430::DADD8mp:
  case MSP430::DADD8mr:
  case MSP430::DADD8rc:
  case MSP430::DADD8ri:
  case MSP430::DADD8rm:
  case MSP430::DADD8rn:
  case MSP430::DADD8rp:
  case MSP430::DADD8rr:
  // End DADD
  // Format-II Instructions
  case MSP430::RRA16m:
  case MSP430::RRA16n:
  case MSP430::RRA16p:
  case MSP430::RRA16r:
  case MSP430::RRA8m:
  case MSP430::RRA8n:
  case MSP430::RRA8p:
  case MSP430::RRA8r:
  case MSP430::RRC16m:
  case MSP430::RRC16n:
  case MSP430::RRC16p:
  case MSP430::RRC16r:
  case MSP430::RRC8m:
  case MSP430::RRC8n:
  case MSP430::RRC8p:
  case MSP430::RRC8r:
  // End Format-II
  case MSP430::SEXT16m:
  case MSP430::SEXT16n:
  case MSP430::SEXT16p:
  case MSP430::SEXT16r:
  case MSP430::SUB16mc:
  case MSP430::SUB16mi:
  case MSP430::SUB16mm:
  case MSP430::SUB16mn:
  case MSP430::SUB16mp:
  case MSP430::SUB16mr:
  case MSP430::SUB16rc:
  case MSP430::SUB16ri:
  case MSP430::SUB16rm:
  case MSP430::SUB16rn:
  case MSP430::SUB16rp:
  case MSP430::SUB16rr:
  case MSP430::SUB8mc:
  case MSP430::SUB8mi:
  case MSP430::SUB8mm:
  case MSP430::SUB8mn:
  case MSP430::SUB8mp:
  case MSP430::SUB8mr:
  case MSP430::SUB8rc:
  case MSP430::SUB8ri:
  case MSP430::SUB8rm:
  case MSP430::SUB8rn:
  case MSP430::SUB8rp:
  case MSP430::SUB8rr:
  case MSP430::SUBC16mc:
  case MSP430::SUBC16mi:
  case MSP430::SUBC16mm:
  case MSP430::SUBC16mn:
  case MSP430::SUBC16mp:
  case MSP430::SUBC16mr:
  case MSP430::SUBC16rc:
  case MSP430::SUBC16ri:
  case MSP430::SUBC16rm:
  case MSP430::SUBC16rn:
  case MSP430::SUBC16rp:
  case MSP430::SUBC16rr:
  case MSP430::SUBC8mc:
  case MSP430::SUBC8mi:
  case MSP430::SUBC8mm:
  case MSP430::SUBC8mn:
  case MSP430::SUBC8mp:
  case MSP430::SUBC8mr:
  case MSP430::SUBC8rc:
  case MSP430::SUBC8ri:
  case MSP430::SUBC8rm:
  case MSP430::SUBC8rn:
  case MSP430::SUBC8rp:
  case MSP430::SUBC8rr:
  // Format-II Instructions
  case MSP430::SWPB16m:
  case MSP430::SWPB16n:
  case MSP430::SWPB16p:
  case MSP430::SWPB16r:
  // End Format-II Instructions
  case MSP430::XOR16mc:
  case MSP430::XOR16mi:
  case MSP430::XOR16mm:
  case MSP430::XOR16mn:
  case MSP430::XOR16mp:
  case MSP430::XOR16mr:
  case MSP430::XOR16rc:
  case MSP430::XOR16ri:
  case MSP430::XOR16rm:
  case MSP430::XOR16rn:
  case MSP430::XOR16rp:
  case MSP430::XOR16rr:
  case MSP430::XOR8mc:
  case MSP430::XOR8mi:
  case MSP430::XOR8mm:
  case MSP430::XOR8mn:
  case MSP430::XOR8mp:
  case MSP430::XOR8mr:
  case MSP430::XOR8rc:
  case MSP430::XOR8ri:
  case MSP430::XOR8rm:
  case MSP430::XOR8rn:
  case MSP430::XOR8rp:
  case MSP430::XOR8rr:
  case MSP430::ZEXT16r:

  // Call Instructions
  // Format-II Instructions
  case MSP430::CALLi:
  case MSP430::CALLm:
  case MSP430::CALLn:
  case MSP430::CALLp:
  case MSP430::CALLr:

  // Move Instructions
  case MSP430::MOV16mc:
  case MSP430::MOV16mi:
  case MSP430::MOV16mm:
  case MSP430::MOV16mn:
  case MSP430::MOV16mr:
  case MSP430::MOV16rc:
  case MSP430::MOV16ri:
  case MSP430::MOV16rm:
  case MSP430::MOV16rn:
  case MSP430::MOV16rp:
  case MSP430::MOV16rr:
  case MSP430::MOV8mc:
  case MSP430::MOV8mi:
  case MSP430::MOV8mm:
  case MSP430::MOV8mn:
  case MSP430::MOV8mr:
  case MSP430::MOV8rc:
  case MSP430::MOV8ri:
  case MSP430::MOV8rm:
  case MSP430::MOV8rn:
  case MSP430::MOV8rp:
  case MSP430::MOV8rr:
  case MSP430::MOVZX16rm8:
  case MSP430::MOVZX16rr8:

  // Miscellaneous Instructions
  // Format-II Instructions
  case MSP430::POP16r:
  case MSP430::PUSH16c:
  case MSP430::PUSH16i:
  case MSP430::PUSH16r:
  case MSP430::PUSH8r:

  // Control Flow Instructions
  // Format-III Instructions
  case MSP430::Bi:
  case MSP430::Bm:
  case MSP430::Br:
  case MSP430::JMP:
  case MSP430::RET:
  case MSP430::RETI:
  // Conditional branches
  case MSP430::JCC:
    break;
    // End Format-III Instructions

  case MSP430::Rrcl16:   // Pseudo
  case MSP430::Rrcl8:    // Pseudo
  case MSP430::Select16: // Pseudo
  case MSP430::Select8:  // Pseudo
  case MSP430::Shl16:    // Pseudo
  case MSP430::Shl8:     // Pseudo
  case MSP430::Sra16:    // Pseudo
  case MSP430::Sra8:     // Pseudo
  case MSP430::Srl16:    // Pseudo
  case MSP430::Srl8:     // Pseudo
    errs() << "PSEUDO Inst: " << I << "\n";
    assert(0 && "Found pseudo instruction, which should be lowered by now!");
    break;
  case TargetOpcode::CFI_INSTRUCTION:
    // TODO We should be able to ignore those but better make sure
    // errs() << "Found CFI\n";
    break;
  case TargetOpcode::DBG_VALUE:
  case TargetOpcode::DBG_LABEL:
  case TargetOpcode::DBG_INSTR_REF:
  case TargetOpcode::DBG_PHI:
  case TargetOpcode::DBG_VALUE_LIST:
  case TargetOpcode::INLINEASM:
    // Debug instructions can be ignored for timing analysis
    break;
  default:
    errs() << "UNKNOWN: " << I << "\n";
    // TODO Handle frame-setup CFI_INSTRUCTION
    assert(0 && "Found unknown instruction");
    break;
  }
}

bool MSP430Target::isControlFlowMnemonic(StringRef Mnemonic) const {
  // MSP430 jumps (real + emulated), plus call and br.
  static const char *const CF[] = {"jmp", "jeq", "jz",   "jne", "jnz",
                                   "jc",  "jhs", "jnc",  "jlo", "jn",
                                   "jge", "jl",  "call", "br"};
  for (const char *M : CF)
    if (Mnemonic == M)
      return true;
  return false;
}

std::optional<uint64_t>
MSP430Target::resolveBranchTarget(StringRef Mnemonic, StringRef Comment) const {
  if (!isControlFlowMnemonic(Mnemonic))
    return std::nullopt;
  StringRef C = Comment.trim();
  // Jumps print "abs 0x4056"; calls/br print "#0x402c".
  if (C.consume_front("abs"))
    C = C.trim();
  else if (C.consume_front("#"))
    C = C.trim();
  else
    return std::nullopt;
  uint64_t Target = 0;
  if (C.getAsInteger(0, Target)) // 0 => honour the "0x" prefix
    return std::nullopt;
  return Target;
}

} // namespace llta
