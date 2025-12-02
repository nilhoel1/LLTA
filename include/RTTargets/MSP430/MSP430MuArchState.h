#include "RTTargets/MuArchStateGraph.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <cassert>
namespace llvm {

struct MSP430MuArchState : public MuArchState {
  MachineBasicBlock &MBB; // Reference to the corresponding MachineInstr
  unsigned int Lat;       // Latency produced by the instruction
  // TODO Add FRAM Cache State

  // Constructor to initialize the Node
  MSP430MuArchState(MachineBasicBlock &MBB, unsigned int Lat)
      : MuArchState(Lat, Lat), MBB(MBB), Lat(Lat) {}
};

} // end namespace llvm
