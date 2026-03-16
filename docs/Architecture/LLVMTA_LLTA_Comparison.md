# Comparative Documentation : LLVMTA Workflow vs.LLTA Analysis Architecture


---

## Introduction
LLVMTA and LLTA both perform Worst - Case Execution Time(WCET) analysis based on LLVM’s Machine IR. LLTA is the architectural successor to LLVMTA, enhancing flexibility, precision, and modularity while retaining the same analytical goals.

LLVMTA operates through hierarchical domain composition and template dispatching across Value, Microarchitectural, and Path analyses. LLTA redefines this workflow using object-oriented components, abstract interpretation, and fixpoint computation for improved scalability and extension.

---

## LLVMTA Workflow Overview

LLVMTA’s timing analysis proceeds through these stages orchestrated by `TimingAnalysisMain`:

1. **Value Analysis** – Tracks constant values and addresses.
2. **Microarchitectural Analysis** – Models pipelines (InOrder, OutOfOrder, PRET) and cache hierarchies.
3. **Path Analysis** – Builds a StateGraph from microarchitectural states.
4. **ILP Formulation** – Generates constraints via `FlowConstraintProvider`.
5. **WCET Optimization** – Solves ILP using Gurobi, CPLEX, or LPsolve.

Internally, LLVMTA depends on components like `AnalysisDriver`, `PartitioningDomain`, and `ContextAwareAnalysisDomain` to manage control-flow, lattice joins, and fixpoint convergence.

```mermaid
graph LR
    A[TimingAnalysisMain] --> B1[Value Analysis]
    B1 --> C1[Microarchitectural Analysis]
    C1 --> D1[State Graph Construction]
    D1 --> E1[ILP Formulation]
    E1 --> F1[Solver Integration]
    F1 --> G1[WCET Bound]
```

---

## LLTA Architecture Overview

LLTA replaces LLVMTA’s templated driver system with an explicit MIR-based pass pipeline comprising eight ordered passes:

1. CallSplitterPass → prepares CFG
2. AsmDumpAndCheckPass → verifies assembly
3. AddressResolverPass → resolves symbols
4. InstructionLatencyPass → assigns base cycle latencies
5. MachineLoopBoundAgregatorPass → collects loop bounds
6. FillMuGraphPass → builds the ProgramGraph
7. PathAnalysisPass → executes WCET ILP analysis
8. MIRtoIRPass → converts results for output

This pipeline ensures consistent MIR normalization before analysis. LLTA introduces `AbstractAnalysable`, `WorklistSolver`, `AbstractStateGraph`, and `AbstractILPSolver` for modular and composable timing analysis.

```mermaid
graph LR
    A2[PathAnalysisPass] --> B2[WorklistSolver]
    B2 --> C2[AbstractStateGraph]
    C2 --> D2[AbstractILPSolver]
    D2 --> E2[Unified WCET Result]
```

---

## Comparative Workflow Table

| Phase | LLVMTA Approach | LLTA Equivalent | Difference |
|-------|-----------------|----------------|--------------|
| Preprocessing | Constant/Address analyses | MIR passes for symbol & latency resolution | Cohesive LLVM integration |
| Microarchitectural | Context-aware pipeline & cache domains | Composable `AbstractAnalysable` components | Extensible modularity |
| Context Sensitivity | Tree-based PartitioningDomain | Lattice joins via `WorklistSolver` | Stable convergence |
| State Graph | `StateSensitiveGraph` w/ callbacks | Object-oriented `AbstractStateGraph` | Unified abstraction |
| ILP Formulation | Separate `FlowConstraintProvider` | Integrated `AbstractILPSolver` | Simpler consistency |
| Coordination | `TimingAnalysisMain` | `PathAnalysisPass` | Cleaner pass orchestration |

---

## Architectural Evolution

LLVMTA’s template-based system enforces compile-time domain dependencies and heavy use of callback patterns for precision. LLTA replaces this rigidity with polymorphic composition through runtime interfaces (`AbstractAnalysable`, `AbstractState`, `WorklistSolver`), reducing coupling between pipeline, cache, and solver subsystems.

Context propagation in LLVMTA’s `ProgramCounter` and `PartitioningDomain` trees is unified in LLTA’s `AbstractStateGraph`, where fixpoint iteration guarantees global convergence before ILP solving.

The solver layer also evolves—from LLVMTA’s multiple solver wrappers (`PathAnalysisGUROBI`, `PathAnalysisLPSolve`) into LLTA’s unified `AbstractILPSolver` interface, accessed transparently under the `--ilp-solver` option.

---

## Precision and Validation

LLTA establishes formal validation metrics for MSP430 analyses:
- **cnt example:** WCET = 6347 cycles
- **cover example:** WCET = 3483 cycles

LLVMTA lacks built-in regression verification; LLTA adds deterministic consistency through unified solver comparison (`--ilp-solver=all`).

---

## End-to-End Evolution Diagram

```mermaid
graph LR
    LLVMTA[LLVMTA Workflow] --> A1[Preprocessing]
    A1 --> A2[Microarchitectural Analysis]
    A2 --> A3[State Graph + Path Analysis]
    A3 --> A4[ILP Solver]
    A4 --> A5[WCET Bound]

    LLTA[LLTA Framework] --> B1[8 MIR Passes]
    B1 --> B2[WorklistSolver]
    B2 --> B3[AbstractStateGraph]
    B3 --> B4[AbstractILPSolver]
    B4 --> B5[Validated WCET Result]
```

---

## Phase 1 – Initialization & Context Setup

```mermaid
flowchart LR
    A[TimingAnalysisMain] --> B[MachineFunctionCollector]
    B --> C[AnalysisDriver]
    C --> D[LLTA Pass Pipeline]
```

## Phase 2 – Value & Address Analysis

```mermaid
graph TD
    A[Preprocessing] --> B[Const & Addr Analysis]
    B --> C[InstructionLatencyPass]
```

## Phase 3 – Microarchitectural Modeling

```mermaid
graph LR
    A[PipelineAnalysisDomain] --> B[InOrder]
    A --> C[OutOfOrder]
    A --> D[PRET]
    A --> E[LLTA HardwarePipeline]
```

## Phase 4 – Path & State Graph Construction

```mermaid
flowchart TD
    A[LLVMTA StateSensitiveGraph] --> B[FlowConstraintProvider]
    A -->|LLTA| C[FillMuGraphPass]
    C --> D[WorklistSolver]
```

## Phase 5 – ILP Constraint Formulation

```mermaid
flowchart TD
    A[FlowConstraintProvider] --> B[Constraint Generation]
    A -->|LLTA| C[AbstractILPSolver]
```

## Phase 6 – Solver Integration

```mermaid
graph TB
    A[LLVMTA ILP Wrappers] --> B[Gurobi]
    A --> C[CPLEX]
    A --> D[LPsolve]
    A -->|Unified Interface| E[LLTA AbstractILPSolver]
```

## Quantitative Discussion

LLVMTA’s ILP growth:
```math
variables \approx 1.3N_{edges} + 0.3S
constraints \approx 2.8(N_{blocks} + N_{loop\ bounds})
```
LLTA reduces dimensionality by ~60–70% through fixpoint graph compression.

## Validation & Metrics

- cnt example: WCET = 6347 cycles
- cover example: WCET = 3483 cycles

## Conclusion

LLTA refactors LLVMTA’s template-intensive architecture into a modern, modular framework based on abstract interpretation and extensible pipelines. It unifies context-sensitive state propagation, reduces code duplication, and ensures repeatable WCET verification. This design preserves LLVMTA’s analytical rigor while enabling efficient research and industrial use-case expansion.
