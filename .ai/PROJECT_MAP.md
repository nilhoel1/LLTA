# Project Map

## Key Directories
- **`.` (Root)**: Contains build scripts (`config.sh`) and the main driver (`LLTA.cpp`).
- **`lib/`**: Core library code.
    - **`lib/MIRPasses`**: Custom Machine IR analyses.
        - `TimingAnalysisPasses.cpp`: Defines the `getTimingAnalysisPasses` pipeline factory.
        - `WCETAnalysisPipeline.cpp`: Implements the "Prep-Pass Pipeline" (IR canonicalization).
        - `InstructionLatencyPass.cpp`: Assigns architectural latencies to instructions.
        - `PathAnalysisPass.cpp`: Constructs and solves the WCET ILP problem (module-level).
        - `FillMuGraphPass.cpp`: Transforms the CFG into a Program Graph.
        - `MachineLoopBoundAgregatorPass.cpp`: Aggregates loop info (header, latch, bounds).
        - `CallSplitterPass.cpp`: Simplifies CFG by splitting basic blocks at call sites.
        - `AdressResolverPass.cpp`: Resolves symbolic addresses for memory analysis.
        - `AsmDumpAndCheckPass.cpp`: Verification pass that dumps assembly.
    - **`lib/RTTargets`**: Hardware modeling.
        - `MSP430/MSP430MuArchState.cpp`: MSP430-specific microarchitectural state model.
        - `MSP430/MSP430Pipeline.cpp`: MSP430 pipeline stages.
        - `ProgramGraph.cpp`: System-independent graph representation of program structure.
    - **`lib/ILP`**: Optimization Solvers.
        - `AbstractGurobiSolver.cpp`: Abstract State ILP solver using Gurobi.
        - `AbstractHighsSolver.cpp`: Abstract State ILP solver using HiGHS.
        - `GurobiSolver.cpp`: (Legacy) Interface to the commercial Gurobi Optimizer.
        - `HighsSolver.cpp`: (Legacy) Interface to the open-source HiGHS solver.
        - `ILPSolver.cpp`: Abstract base class for solvers.
    - **`lib/Utility`**:
        - `Options.cpp`: Command-line option definitions and parsing helpers.
    - **`lib/Analysis`**: Abstract Analysis Framework.
        - `AbstractStateGraph.cpp`: Graph of abstract states.
        - `GraphAdapter.cpp`: Adapts AbstractStateGraph to ProgramGraph for ILP.
        - `WorklistSolver.cpp`: Generic fixpoint algorithm.
        - `PipelineAnalysis.cpp`: Core domain logic.
        - `SystemState.cpp`: Lattice element implementation.
        - `HardwareStrategies.cpp`: Cache and BP models.
    - **`lib/Pipeline`**: Hardware Pipeline Simulation.
        - `HardwarePipeline.cpp`: Cycle-accurate pipeline model implementation.
- **`include/`**: Header files for the libraries.
    - `include/Analysis/`: Headers for abstract base classes (`AbstractState`, `AbstractAnalysis`, `WorklistSolver`) and concrete implementations (`SystemState`, `PipelineAnalysis`, `HardwareStrategies`, `MicroArchitectureAnalysis`).
    - `include/Pipeline/`: Hardware pipeline headers (`HardwarePipeline.h`, `AbstractHardwareStage`).
    - `include/ILP/`: Headers for ILP solvers (`AbstractILPSolver`, `AbstractGurobiSolver`, `AbstractHighsSolver`).
- **`externalDeps/`**: Downloaded external dependencies (LLVM source).
- **`scripts/`**: Helper scripts for downloading and patching LLVM.
- **`build/`**: Build artifacts (generated).
- **`tests/`**: Integration and regression tests.
    - **`regression_test.py`**: Mandatory regression test suite (tests `cnt`, `cover`, `cnt-all-solvers`).
    - **`msp430/`**: Test cases (`cnt`, `cover`, etc).

## Entry Point
- **`LLTA.cpp`**: The main driver for the `llta` tool, setting up the pass pipeline and invoking the analysis.

## Documentation
- **`.ai/ARCHITECTURE.md`**: System architecture and pass pipeline.
- **`.ai/PROJECT_MAP.md`**: This file - directory and file reference.
- **`.ai/USAGE_GUIDE.md`**: How to use LLTA and extend it with new analyses.
- **`.ai/TECH_STACK.md`**: Technologies and dependencies.
- **`.ai/CODING_RULES.md`**: Style and naming conventions.
- **`.ai/MAINTENANCE_PROTOCOL.md`**: Documentation update requirements.

