# Thesis Proposal: Automated Grey-Box Characterization of RISC-V Pipelines

**Candidate**: [Student Name]
**Context**: LLTA Framework Extension
**Target**: ESP32-C6 (RISC-V)

## 1. Abstract
Static timing analysis (WCET) requires precise microarchitectural models. However, detailed pipeline specifications for embedded RISC-V cores are often proprietary or undocumented. Manual reverse engineering is error-prone and labor-intensive. This thesis proposes a **Grey-Box Learning Framework** integrated into LLTA. The framework will automate the generation of micro-benchmarks, their execution on physical hardware, and the parameterization of an abstract pipeline model using an optimization loop.

## 2. Research Question
*Can a generic, parameterized 4-stage pipeline model be automatically tuned to match the behavior of a physical RISC-V processor (ESP32-C6) with high fidelity (>95% accuracy) using a "Generate-Measure-Learn" loop?*

## 3. Methodology

The proposed system consists of three distinct components orchestrated by an external driver (Python).

### 3.1 Component A: The Parameterized Model (White/Grey Box)
Unlike traditional static models with hardcoded latencies, we introduce a **Configurable Data-Driven Pipeline** in LLTA that adheres to the `HardwarePipeline` and `AbstractHardwareStage` interfaces.

*   **Implementation**:
    *   Class `ConfigurableGenericStage` inherits `llvm::AbstractHardwareStage`.
    *   Class `ConfigurablePipelineFactory` reads JSON and assembles a `HardwarePipeline`.
*   **Mechanism**:
    *   Instead of hardcoded `ESP32C6...Stage` classes, use a single `GenericStage` class that takes a `StageConfig` struct in its constructor (latency, buffer size, resource usage).
    *   Forwarding logic is handled by a shared `HazardModel` or `Scoreboard` object passed to all generic stages.
*   **Parameters (JSON)**:
    *   `OpcodeLatency[N]`: Execution cycles for each instruction class.
    *   `ForwardingMatrix`: Defines if Stage A can forward to Stage B.
    *   `StageDepths`: FIFO sizes for queues between stages.

### 3.2 Component B: The Generator (LLTA Extension)
A new analysis pass (`MicrobenchmarkGenPass`) will be developed to algorithmically generate "probes"—short assembly sequences designed to isolate specific pipeline features.
* **Latency Probes**: Dependent instruction chains (RAW hazards).
* **Throughput Probes**: Independent instruction streams (Structural hazards).
* **Output**: RISC-V object code or assembly compatible with the ESP-IDF toolchain.

### 3.3 Component C: The Optimization Loop (The Learner)
An external Python orchestrator will close the loop:
1.  **Hypothesize**: Generate an initial `pipeline_config.json`.
2.  **Simulate**: Run LLTA with the config to predict cycle counts for a set of probes.
3.  **Measure**: Dispatch the same probes to the ESP32-C6 (via a UART-based test harness running in IRAM) to capture ground truth.
4.  **Minimize**: Use a minimization algorithm (e.g., Simulated Annealing or Genetic Algorithm) to adjust the `pipeline_config.json` parameters until the Simulation Error $\sum |Predicted - Actual|$ is minimized.

## 4. Work Plan (6 Months)

| Phase                  | Duration    | Deliverable                                                            |
| :--------------------- | :---------- | :--------------------------------------------------------------------- |
| **I. Infrastructure**  | Weeks 1-6   | Parameterized `HardwarePipeline` in C++ & JSON Parser.                 |
| **II. The Harness**    | Weeks 7-10  | ESP32-C6 Firmware that executes code received over UART from RAM/IRAM. |
| **III. The Generator** | Weeks 11-16 | LLVM Pass to emit `MachineInstr` pairs for `RV32IM` opcodes.           |
| **IV. The Loop**       | Weeks 17-20 | Python Orchestrator and Learning Algorithm implementation.             |
| **V. Evaluation**      | Weeks 21-24 | Validation against real-world applications (e.g., DSP filters).        |

## 5. Expected Contribution
1.  **Open Source Tool**: A generic "RISC-V Pipeline Learner" adaptable to other cores (e.g., SiFive, Ibex).
2.  **Dataset**: A verified pipeline configuration for the popular ESP32-C6.
3.  **Methodology**: A documented process for "Grey-Box" WCET modeling using LLVM.

## 6. Challenges
* **Instruction Alignment**: The 'C' extension (16-bit) causes non-deterministic fetch behaviors that are difficult to learn via simple scalar parameters.
* **Toolchain Integration**: Generating valid, linkable object code from LLTA without a full linker setup is non-trivial. (Mitigation: Generate only function bodies, link against a static C harness).

## 7. Implementation Hints

### Connecting to Analysis Framework
The "Simulator" in Component C (Step 3.3.2) is **not** a standalone simulator. It is the **LLTA Analysis Framework** configured with the `ConfigurablePipeline`.

1.  **JSON Parser**: Reads config, instantiates `HardwarePipeline` with `ConfigurableGenericStage` objects.
2.  **Analysis Wrapper**: Wrap this pipeline in `MicroArchitectureAnalysis`.
3.  **Execution**: Run `WorklistSolver` on the generated probe basic blocks. The result (Abstract State) contains the `CycleCount`.

### Automation Hook
The Python orchestrator will need to invoke a specific LLTA tool (e.g., `llta-tune`) that:
1.  Accepts a `--config pipeline.json` argument.
2.  Accepts the probe source/bitcode.
3.  Runs the analysis and outputs the predicted cycle count to `stdout` or a JSON report.