#include "MIRPasses/InstructionLatencyPass.h"
#include <cassert>
#include <optional>
#include <utility>

#include "MCTargetDesc/MSP430MCTargetDesc.h"
#include "TimingAnalysisResults.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

char InstructionLatencyPass::ID = 0;

/**
 * @brief Construct a new Asm Dump And Check Pass:: Asm Dump And Check Pass
 * object
 *
 * @param TM
 */
InstructionLatencyPass::InstructionLatencyPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR), MBBLatencyMap(std::unordered_map<const MachineBasicBlock *, unsigned int>()) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
//bool InstructionLatencyPass::doFinalization(Module &M) { return false; }

/**
 * @brief Iterates over MachineFunction
 *        and dumps its content into a File.
 *
 * @param F
 * @return true
 * @return false
 */
bool InstructionLatencyPass::runOnMachineFunction(MachineFunction &F) {
  if (DebugPrints)
    outs() << "Running InstructionLatencyPass on Function: " << F.getName() << "\n";
  auto Arch = F.getTarget().getTargetTriple().getArch();
  for (auto &MBB : F) {
    // Sum up the latencies of all instructions in the basic block
    unsigned int Latency = 0;
    for (auto &MI : MBB) {
      unsigned int InstructionLatency = 0;
      switch (Arch) {
      case Triple::ArchType::msp430:
        InstructionLatency = getMSP430Latency(MI);
        if (DebugPrints)
          outs() << "Instruction: " << MI << "Latency: " << getMSP430Latency(MI)
                 << "\n";
        break;
      default:
        errs() << "Unknown Arch: " << Arch;
        assert(false && "not implemented");
      }
      Latency += InstructionLatency;
    }
    std::pair<const MachineBasicBlock *, unsigned int> MBBLatencyPair=std::make_pair(&MBB, Latency);
    auto NoDuplicate = MBBLatencyMap.insert(MBBLatencyPair);
    if (!NoDuplicate.second) {
      // If the pair already exists, print a warning
      errs() << "Warning: Duplicate MBB found: " << MBB.getName()
             << " with Latency: " << Latency << "\n";
    }
    assert(NoDuplicate.second && "Duplicate MBB found in MBBLatencyMap");

  }
  TAR.setMBBLatencyMap(MBBLatencyMap);
  return false;
}

// TODO I dont know how the latencies will be represented if the FRAM Controller
// is analysed. Mayube use struct instead if simple unsigned int.

// FIXME Currently we assume CPUx on the MSP430, this should be corrected, when
// llvm also supports the MSP430 CPUX. Issue with this is latencies only hold
// for the upper 64kb of memory on MSP430 CPUx
unsigned int InstructionLatencyPass::getMSP430Latency(const MachineInstr &I) {
  // r = RN, RM
  // m = x(Rn), x(Rm), EDE, &EDE
  // n = @Rn
  // p = @Rn+
  // c, i = #N
  // if two are used first is destination secnd is source
  // E.g.: ADD16rm -> r = Destination, m = Source
  switch (I.getOpcode()) {
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
  case MSP430::Bi:                                  // Emulated
    for (const MachineOperand &MO : I.operands()) { // check for PC
      if (MO.isReg() && MO.getReg() == MSP430::PC)
        return 3;
    }
    return 2;

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
  case MSP430::Bm:                                  // Emulated
    for (const MachineOperand &MO : I.operands()) { // check for PC
      if (MO.isReg() && MO.getReg() == MSP430::PC)
        return 3; // 5 on Non MSP430 non X
    }
    return 3;

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
    for (const MachineOperand &MO : I.operands()) { // check for PC
      if (MO.isReg() && MO.getReg() == MSP430::PC)
        return 2; // 4 on Non MSP430 non X
    }
    return 2;

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
    for (const MachineOperand &MO : I.operands()) { // check for PC
      if (MO.isReg() && MO.getReg() == MSP430::PC)
        return 3; // 4 on Non MSP430 non X
    }
    return 2;

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
  case MSP430::Br:                                  // Emulated
    for (const MachineOperand &MO : I.operands()) { // check for PC
      if (MO.isReg() && MO.getReg() == MSP430::PC)
        return 2; // 3 on Non MSP430 non X
    }
    return 1;
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

  default:
    errs() << "No Latency assigned to Inst: " << I << "\n";
    assert(0 && "Instruction has no Latency!");
  }

  return 0;
}

MachineFunctionPass *createInstructionLatencyPass(TimingAnalysisResults &TAR) {
  return new InstructionLatencyPass(TAR);
}
} // namespace llvm
