# LLTA — Overview & Where to Find What

LLTA is an LLVM-based **WCET (worst-case execution time) timing analyzer**. It
runs as an `llc`-like tool: it consumes LLVM IR / MIR, runs a chain of
MachineFunction passes that annotate timing, builds a program graph, and solves
a WCET ILP. The architecture under analysis is identified by the **LLVM target
triple** and described by an `RTTarget` (see [DESIGN_GUIDELINES.md](DESIGN_GUIDELINES.md)).

## Directory map

| Path | What lives here |
|------|-----------------|
| `LLTA.cpp`, `NewPMDriver.{cpp,h}` | Tool entry point; builds the codegen + analysis pass pipeline. |
| `include/Targets/`, `lib/Targets/` | **Target-specific** code. `RTTarget` interface + `TargetRegistry`; one subdir per target family/device (`MSP430/`, data-only `ESP32-C6/`). Latencies, instruction checks, memory-model passes (FRAM), target options live here. |
| `include/Graph/`, `lib/Graph/` | `ProgramGraph` — the target-agnostic program-graph representation. |
| `include/Analysis/`, `lib/Analysis/` | Reusable analysis framework: abstract-interpretation (`AbstractState`, `WorklistSolver`, `AbstractStateGraph`), pipeline modeling, and the generic cache analysis (`Cache/`). |
| `include/MIRPasses/`, `lib/MIRPasses/` | The generic timing-analysis passes and the pipeline builder (`getTimingAnalysisPasses`). |
| `include/ILP/`, `lib/ILP/` | Abstract ILP solvers (`AbstractGurobiSolver`, `AbstractHighsSolver`) + backend selection. |
| `include/Pipeline/`, `lib/Pipeline/` | Hardware-pipeline simulation building blocks. |
| `include/Utility/`, `lib/Utility/` | Generic CLI options and helpers. |
| `include/TimingAnalysisResults.h` | Shared results container threaded through all passes; holds the active `RTTarget`. |
| `cmake/` | Non-root CMake helpers (`FindGUROBI.cmake`, `FindHIGHS.cmake`). |
| `tests/` | `regression_test.py`, MSP430 benchmarks (`tests/msp430/`), unit tests (`tests/unit/`). |
| `clang-plugin/` | Clang plugin that extracts loop bounds. |

## Pass pipeline (built by `getTimingAnalysisPasses(Triple)`)

The triple resolves the active `RTTarget`, which is installed once and queried
by every pass. The skeleton is generic; the target splices in its own
memory-model passes:

1. **CallSplitterPass** — prepares the CFG (splits calls). *(In `-llc` mode, the pipeline stops here.)*
2. **AsmDumpAndCheckPass** — validates each instruction against the target's model (`RTTarget::checkInstruction`).
3. **AdressResolverPass** — aligns objdump disassembly with MIR; resolves instruction addresses and branch targets (`RTTarget::isControlFlowMnemonic` / `resolveBranchTarget`).
4. **InstructionLatencyPass** — base per-instruction latencies (`RTTarget::getInstructionLatency`) → `MBBLatencyMap`.
5. **\<target memory-model passes\>** — `RTTarget::getMemoryModelPasses` (e.g. MSP430FR's FRAM wait-state + read-cache passes). No-ops unless configured.
6. **MachineLoopBoundAgregatorPass** — loop bounds (SCEV / clang-plugin JSON).
7. **FillMuGraphPass** — builds the `ProgramGraph` from `MBBLatencyMap` + bounds.
8. **PathAnalysisPass** — abstract interpretation over the graph, then solves the WCET ILP (`-ilp-solver auto|gurobi|highs`).

## Build & test

```bash
./config.sh config      # configure (auto-downloads LLVM the first time)
./config.sh build       # build the `llta` tool
python3 tests/regression_test.py   # WCET regression on the MSP430 benchmarks
```

See [DESIGN_GUIDELINES.md](DESIGN_GUIDELINES.md) for conventions, testing, and
how to add a target. The assumptions the design makes about every target are in
[TargetAssumptions.md](TargetAssumptions.md).
