# Project Map

## Key Directories
- **`.` (Root)**: Contains build scripts (`config.sh`) and the main driver (`LLTA.cpp`).
- **`lib/`**: Core library code.
    - **`lib/MIRPasses`**: Custom Machine IR analyses.
        - `TimingAnalysisPasses.cpp`: Defines the `getTimingAnalysisPasses` pipeline factory.
        - `InstructionLatencyPass.cpp`: Assigns architectural latencies to instructions.
        - `PathAnalysisPass.cpp`: Constructs and solves the WCET ILP problem.
        - `FillMuGraphPass.cpp`: Transforms the CFG into a Microarchitectural State Graph (MASG).
        - `MachineLoopBoundAgregatorPass.cpp`: Aggregates loop info (header, latch, bounds).
        - `CallSplitterPass.cpp`: Simplifies CFG by splitting basic blocks at call sites.
        - `AdressResolverPass.cpp`: Resolves symbolic addresses for memory analysis.
        - `AsmDumpAndCheckPass.cpp`: Verification pass that dumps assembly.
    - **`lib/RTTargets`**: Hardware modeling.
        - `MSP430/MSP430MuArchState.cpp`: MSP430-specific microarchitectural state model.
        - `MuArchStateGraph.cpp`: System-independent graph representation of hardware states.
    - **`lib/ILP`**: Optimization Solvers.
        - `GurobiSolver.cpp`: Interface to the commercial Gurobi Optimizer.
        - `HighsSolver.cpp`: Interface to the open-source HiGHS solver.
        - `ILPSolver.cpp`: Abstract base class for solvers.
    - **`lib/Utility`**:
        - `Options.cpp`: Command-line option definitions and parsing helpers.
    - **`lib/Analysis`** (Planned):
        - `WorklistSolver.cpp`: Generic fixpoint algorithm.
        - `PipelineAnalysis.cpp`: Core domain logic.
        - `SystemState.cpp`: Lattice element implementation.
        - `HardwareStrategies.cpp`: Cache and BP models.
- **`include/`**: Header files for the libraries.
    - `include/llta/Analysis/`: Headers for `SystemState`, `PipelineAnalysis`, `HardwareStrategies`.
    - `include/llta/Solver/`: Header for `WorklistSolver`.
- **`externalDeps/`**: Downloaded external dependencies (LLVM source).
- **`scripts/`**: Helper scripts for downloading and patching LLVM.
- **`build/`**: Build artifacts (generated).

## Entry Point
- **`LLTA.cpp`**: The main driver for the `llta` tool, setting up the pass pipeline and invoking the analysis.
