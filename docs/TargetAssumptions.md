# Target Assumptions

What the LLTA design assumes about **any** target it analyzes. A new target must
fit these or the generic layer needs extending. (Device-specific assumptions
live with each target, e.g. `lib/Targets/ESP32-C6/ESP32-C6-Assumptions.md`.)

## Identity

- The architecture is identified by the **LLVM target triple**; `Triple::ArchType`
  selects a target family, and a per-family device-resolution step picks the
  concrete device. One LLVM arch may cover a family of devices (MSP430 → many
  MSP430(FR) parts); the model accounts for this with a family/device split.

## Timing model

- A per-instruction **base latency** in CPU cycles exists and can be derived from
  the `MachineInstr` opcode (and, where needed, its operands), assuming zero
  memory wait states. Memory/cache penalties are layered on top by the target's
  memory-model passes, not folded into the base latency.
- The cost of a basic block is the **sum** of its instruction costs
  (`MBBLatencyMap`); the WCET is the maximum-cost path through the
  `ProgramGraph`, bounded by loop bounds, solved as an ILP. The WCET is an
  **upper bound** (round solver objectives up/to-nearest, never down).
- Loop bounds come from SCEV or a clang-plugin JSON, not from the target.

## Instruction stream

- The program is fully **lowered** before analysis: no un-lowered pseudo
  instructions remain (`checkInstruction` asserts otherwise). Debug/CFI
  pseudos carry zero latency.
- Each instruction fetches a bounded number of fixed-width code words
  (`getMaxInstructionWords`), used to attribute fetch-side memory penalties.

## Disassembly / address resolution

- A linked **objdump-style disassembly** can be aligned with the MIR to recover
  each instruction's real address and static branch/call targets. The target
  knows its control-flow mnemonics (`isControlFlowMnemonic`) and how its
  toolchain prints static targets in trailing comments (`resolveBranchTarget`).
- Indirect transfers may have no static target; analysis must tolerate that.

## Memory model (optional, target-contributed)

- A target may contribute extra MachineFunction passes
  (`getMemoryModelPasses`) that refine `MBBLatencyMap` with memory penalties
  (e.g. wait states, a read cache). These are **sound over-approximations** and
  are no-ops unless explicitly configured, so default runs are unaffected.
- The generic cache analysis (`Analysis/Cache/`) is reusable across targets; only
  the access mapper (which instruction accesses which address) is target-specific.
