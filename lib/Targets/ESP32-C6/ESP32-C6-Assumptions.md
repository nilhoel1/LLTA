# ESP32-C6 Target — Model & Assumptions

This directory is a **data-only target home**: it holds the reverse-engineering
knowledge for the Espressif ESP32-C6 (RISC-V HP core), captured for a future
LLTA target implementation. There is **no `RTTarget` subclass yet** and the
ESP32-C6 is **not registered** — MSP430FR5994 is currently the only active
target. When the target is implemented, the empirical model and the assumptions
below are its source of truth.

## Files

- `ESP32-C6-Model.json` — the derived timing model (memory hierarchy,
  per-class instruction latencies, branch-prediction costs, hazard penalties).
- `results.csv` — raw empirical per-instruction latencies from LLTA-Bench
  measurements on real hardware.

## Target overview

- **ISA**: RV32IMAC (RISC-V 32-bit; Integer, Multiply, Atomic, Compressed) →
  LLVM arch `riscv32`.
- **Clock**: 160 MHz.
- **Pipeline**: 4 stages (Fetch, Decode, Execute, Write-Back/Memory),
  single-issue (hypothesis from high-level docs; to be confirmed).
- **Branch prediction**: static BTFN (Backward-Taken, Forward-Not-Taken) in the
  current model.

## Model assumptions (from `ESP32-C6-Model.json`)

- **Memory hierarchy**
  - Internal SRAM `0x40800000–0x4087FFFF`: tightly coupled, no cache, 3-cycle
    access latency.
  - External flash `0x42000000–0x42FFFFFF`: cached via SPI0 — 32 KB, 4-way,
    32-byte lines, LRU, 4-cycle hit, **348-cycle miss penalty**, read-only.
- **Instruction timing (cycles)**: ALU/MUL(low)/store/nop = 1; MULH = 2;
  sext/zext = 2; load (SRAM) = 3; atomic = 6; DIV/REM = 10 (non-pipelined,
  blocks issue).
- **Control flow (static BTFN)**: backward-taken = 2, forward-not-taken = 1,
  unconditional jump (JAL/J, always flushes) = 3, mispredict (either direction)
  = 4; default WCET branch cost = 4.
- **Penalties**: unaligned load/store +1; DIV serializes the pipeline
  (cost = 10 + next-instruction cost).

## Implementation plan (folded from the former ESP32C6 research plan)

When implementing the target (a `RTTarget` subclass registered for
`riscv32`, plus its microarchitecture model):

1. **Pipeline infrastructure** — implement an `AbstractHardwareStage` per stage
   (`ESP32C6Fetch/Decode/Execute/WriteBack`); a factory assembles a
   `HardwarePipeline`; wrap it in `MicroArchitectureAnalysis` so the
   `WorklistSolver` drives the fixpoint. Decode holds a scoreboard for RAW
   hazards; Fetch needs a byte queue for compressed (16/32-bit) instructions.
2. **Micro-benchmarking (reverse engineering)** — bare-metal sequences timed via
   the `rdcycle` CSR (interrupts disabled, cache pre-warmed): latency matrix,
   throughput matrix, forwarding/stall behavior, branch (mis)prediction cost.
   This produced `results.csv` / `ESP32-C6-Model.json`.
3. **Integration & validation** — implement the target under `Targets/ESP32-C6/`,
   register it in the target registry, validate predictions against the
   measurements (target error < 5%).

### Risks

- Flash I-cache nondeterminism — mitigate by placing benchmark code in IRAM
  (`IRAM_ATTR`) for core pipeline characterization.
- Compressed instructions complicate Fetch — start with RV32IM (no `C`) for
  initial validation.
