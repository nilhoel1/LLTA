# Architecture

## Analysis Pipeline (Pass Order)
The `llta` tool executes the following MachineFunction passes in order (defined in `lib/MIRPasses/TimingAnalysisPasses.cpp`):

1. **`CallSplitterPass`**: Prepares the CFG by splitting calls if necessary.
2. **`AsmDumpAndCheckPass`**: Dumps assembly for verification.
3. **`AdressResolverPass`**: Resolves symbol addresses.
4. **`InstructionLatencyPass`**: Assigns base latencies to instructions using Target models (e.g., MSP430).
5. **`MachineLoopBoundAgregatorPass`**: Collects loop bounds.
6. **`FillMuGraphPass`**: Builds the Program Graph (`ProgramGraph`).
7. **`PathAnalysisPass`**: Performs the final WCET analysis using ILP (module-level in `doFinalization`).
8. **`MIRtoIRPass`**: Converts internal results back to a form suitable for output/reporting.

> [!NOTE]
> **Debug Mode**: If the `DebugIR` option is enabled, the entire pipeline is replaced by a single **`DebugIRPass`** which dumps the IR for inspection.

## Key Components

### PathAnalysisPass & InstructionLatencyPass
- **`InstructionLatencyPass`**: Uses `MSP430` opcode switch-cases (in `getMSP430Latency`) to assign cycle counts to individual instructions. It feeds latency data into the `TimingAnalysisResults`.
- **`PathAnalysisPass`**: Orchestrates the WCET analysis in `doFinalization` (module-level). It runs **`WorklistSolver`** on the `ProgramGraph` to build an `AbstractStateGraph`, then uses **`AbstractILPSolver`** (Gurobi or HiGHS) to solve the longest path problem.

## Dual Analysis Architecture

LLTA supports two analysis paths that should produce identical results:

| Analysis Path | Graph Type           | Solver                                        | Description                            |
| ------------- | -------------------- | --------------------------------------------- | -------------------------------------- |
| **Legacy**    | `ProgramGraph`       | `GurobiSolver`, `HighsSolver`                 | Direct ILP on static graph             |
| **Abstract**  | `AbstractStateGraph` | `AbstractGurobiSolver`, `AbstractHighsSolver` | Worklist-based abstract interpretation |

### Comparison Mode (`--ilp-solver=all`)
When using `--ilp-solver=all`, LLTA runs all 4 solver configurations and outputs a unified comparison table:

```
+----------+-----------+------------+---------+-------------+----------------+
| Type     | Solver    | Available  | Success | WCET (cyc)  | Time (ms)      |
+----------+-----------+------------+---------+-------------+----------------+
| Legacy   | Gurobi    | Yes        | Yes     |        6347 |          1.468 |
| Legacy   | HiGHS     | Yes        | Yes     |        6347 |          1.013 |
| Abstract | Gurobi    | Yes        | Yes     |        6347 |          0.938 |
| Abstract | HiGHS     | Yes        | Yes     |        6347 |          0.654 |
+----------+-----------+------------+---------+-------------+----------------+
```

## Abstract Analysis Framework

### Core Components
- **`AbstractState`**: Base class for lattice elements (join, meet, equals, clone).
- **`AbstractAnalysable`**: Interface for transfer functions (`process`, `getInitialState`).
- **`PipelineAnalysis`**: Composable analysis pipeline that orchestrates sub-analyses.
- **`WorklistSolver`**: Generic fixpoint algorithm that iterates until convergence.
- **`AbstractStateGraph`**: Result of abstract interpretation (nodes with states and costs).

### Extending the Framework
See `.ai/USAGE_GUIDE.md` for detailed instructions on:
- Adding new target architectures
- Creating custom analyses
- Integrating new pipeline stages

## Hardware Pipeline Simulation

### Overview
The `HardwarePipeline` class (in `include/Pipeline/`) provides a cycle-accurate simulation of a CPU pipeline. It is integrated with the `AbstractAnalysable` interface via `MicroArchitectureAnalysis` (in `include/Analysis/`).

### Core Components
- **`AbstractHardwareStage`**: Interface for pipeline stages (Fetch, Decode, Execute, etc.). Each stage implements `cycle()`, `isReady()`, `execute()`, `getBusyCycles()`, and `clone()`.
- **`HardwarePipeline`**: Holds a vector of stages. Provides `injectInstruction()`, `cycle()`, `isEmpty()`, `isRetired()`, and `convertCyclesToFastForward()` for fast simulation.
- **`MicroArchState`**: An `AbstractState` that wraps the `HardwarePipeline`.
- **`MicroArchitectureAnalysis`**: An `AbstractAnalysable` that uses the pipeline to compute cycle costs.

### Simulation Strategy
- **Inject & Retire**: Instructions are injected into the first stage and the simulation runs until the instruction retires from the last stage.
- **Fast Forward**: Uses `getBusyCycles()` to skip ahead when all stages have known busy times.
- **Dependency Handling**: Scoreboard or similar mechanisms (to be implemented in concrete stages) handle register dependencies.

### Future Extensibility
- **Branch Misprediction**: Add `flush()` method to clear the pipeline.
- **Shared Resources**: Add a `ResourceManager` for bus arbitration between Fetch/Execute.



