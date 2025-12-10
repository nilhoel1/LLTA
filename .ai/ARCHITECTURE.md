# Architecture

## Analysis Pipeline (Pass Order)
The `llta` tool executes the following MachineFunction passes in order (defined in `lib/MIRPasses/TimingAnalysisPasses.cpp`):

1. **`CallSplitterPass`**: Prepares the CFG by splitting calls if necessary.
2. **`AsmDumpAndCheckPass`**: Dumps assembly for verification.
3. **`AdressResolverPass`**: Resolves symbol addresses.
4. **`InstructionLatencyPass`**: Assigns base latencies to instructions using Target models (e.g., MSP430).
5. **`MachineLoopBoundAgregatorPass`**: Collects loop bounds.
6. **`FillMuGraphPass`**: Builds the Microarchitectural State Graph (MASG).
7. **`PathAnalysisPass`**: Performs the final WCET analysis using ILP.
8. **`MIRtoIRPass`**: Converts internal results back to a form suitable for output/reporting.

> [!NOTE]
> **Debug Mode**: If the `DebugIR` option is enabled, the entire pipeline is replaced by a single **`DebugIRPass`** which dumps the IR for inspection.

## Key Components

### PathAnalysisPass & InstructionLatencyPass
- **`InstructionLatencyPass`**: Uses `MSP430` opcode switch-cases (in `getMSP430Latency`) to assign cycle counts to individual instructions. It feeds latency data into the `TimingAnalysisResults`.
- **`PathAnalysisPass`**: The consumer of the graph and latency data. It constructs an ILP (Integer Linear Programming) problem to find the longest path (Worst-Case Execution Time) through the code. It uses **Gurobi** or **HiGHS** to solve this optimization problem.

## Hybrid LLTA Analysis Engine

> [!IMPORTANT]
> The project is transitioning to a **Hybrid Analysis Engine** that combines a static Worklist Solver with dynamic Hardware Strategies.
> See [ANALYSIS_DESIGN.md](ANALYSIS_DESIGN.md) for the detailed blueprint.

- **Solver**: Generic fixpoint engine.
- **Pipeline Domain**: Logic for hazard detection.
- **Strategies**: Pluggable C++ models for Cache and Branch Prediction.