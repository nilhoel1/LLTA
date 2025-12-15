# Research Plan: ESP32-C6 Microarchitectural Modeling

## 1. Objective
To implement a cycle-accurate timing model of the **Espressif ESP32-C6 (RISC-V HP Core)** within the LLTA framework. The lack of detailed microarchitectural documentation requires a "Black Box" characterization approach to derive pipeline latencies, hazard logic, and branch prediction behavior.

## 2. Target Architecture Overview
* **ISA**: RV32IMAC (RISC-V 32-bit, Integer, Multiply, Atomic, Compressed).
* **Pipeline Depth**: 4 Stages (Fetch, Decode, Execute, Write-Back/Memory) [Hypothesis based on high-level docs].
* **Dual-Issue**: Unlikely (Single-issue assumed).
* **Branch Prediction**: Dynamic (to be verified) or Static.

## 3. Work Packages

### WP1: The Baseline Pipeline Infrastructure
**Goal**: Adapt LLTA's `HardwarePipeline` to support the specific quirks of the ESP32-C6, conforming to the `include/Pipeline/README.md` and `include/Analysis/README.md` standards.

*   **Task 1.1 (Stage Implementation)**: Implement the `AbstractHardwareStage` interface for each pipeline stage.
    *   *Files*: `lib/Pipeline/ESP32C6Stages.cpp`, `include/Pipeline/ESP32C6Stages.h`
    *   Define `ESP32C6Fetch`, `ESP32C6Decode`, `ESP32C6Execute`, `ESP32C6WriteBack`.
*   **Task 1.2 (Pipeline Assembly)**: Create a factory function to assemble the `HardwarePipeline`.
    *   *Reference*: See "Adding a New Pipeline Stage" in `include/Pipeline/README.md`.
*   **Task 1.3 (Analysis Integration)**: Wrap the assembled pipeline in `MicroArchitectureAnalysis` to integrate with the Abstract Analysis Framework.
    *   *Mechanism*: The `MicroArchitectureAnalysis` acts as the `AbstractAnalysable` adapter for the `HardwarePipeline`.
*   **Task 1.4 (Detail Modeling)**:
    *   *Fetch*: Implement a byte queue for compressed instructions (handle dynamic 16/32-bit fetch).
    *   *Hazards*: Implement a scoreboard mechanism within the `isReady()` logic of the Decode stage to handle Read-After-Write (RAW) stalls.

### WP2: Micro-Benchmarking (Reverse Engineering)
**Goal**: Gather ground truth data to fill the parameters of the WP1 model.
* **Methodology**: Run hand-crafted assembly sequences on bare-metal ESP32-C6 hardware, measuring execution time via the `rdcycle` CSR.
* **The Harness**: A FreeRTOS task running on Core 0 with interrupts disabled (`portDISABLE_INTERRUPTS()`) and cache pre-warmed.

**Experiments:**
1.  **Latency Matrix**: Measure `INST r1, r1, r2` chains for every opcode (ALU, MUL, DIV, Load/Store).
    * *Hypothesis*: `MUL` is >1 cycle. `DIV` is non-blocking but has long latency.
2.  **Throughput Matrix**: Measure `INST r1, r2, r3` (independent operands) to detect pipelining capabilities.
3.  **Forwarding Logic**: Test `ADD r1, ...; ADD ..., r1` vs `ADD r1, ...; NOP; ADD ..., r1`.
    * *Goal*: Determine if operands are forwarded from EX/WB stages to ID/EX, or if stalls occur.
4.  **Branch Cost**: Measure `BNE` (taken) vs `BNE` (not taken) in unrolled loops to determine misprediction penalties.

### WP3: Integration & Validation
**Goal**: Merge the model into LLTA and verify against WP2 data.
* **Implementation**: Create `lib/RTTargets/ESP32C6/` and register the target in `InstructionLatencyPass.cpp`.
* **Validation**: Compare LLTA's `CycleCount` prediction against the hardware measurements.
    * *Success Metric*: Error rate < 5% on the benchmarking suite.

## 4. Technical Risks
* **Nondeterminism**: Flash cache (ICache) hits/misses may noise data.
    * *Mitigation*: Place benchmark code in IRAM (`IRAM_ATTR`) to bypass cache for core pipeline characterization.
* **Compressed Instructions**: The variable instruction length significantly complicates the Fetch stage modeling.
    * *Mitigation*: Start with `RV32IM` (no C extension) for initial validation.

## 5. Implementation Hints

Based on `include/Analysis/README.md` and `include/Pipeline/README.md`:

### Pipeline Stage Template
Each stage (Fetch, Decode, etc.) must implement `llvm::AbstractHardwareStage`.

```cpp
class ESP32C6DecodeStage : public llvm::AbstractHardwareStage {
    // Scoreboard logic for hazards
public:
    void cycle() override;
    bool isReady() const override; // Check for structural/data hazards
    void execute(const llvm::MachineInstr *MI) override;
    // ...
};
```

### Analysis Integration
To perform WCET analysis using this model, you don't run the pipeline manually in the loop. Instead, you register it with the `MicroArchitectureAnalysis`:

1.  **Build Pipeline**: Create a `HardwarePipeline` instance composed of your `ESP32C6...Stage` objects.
2.  **Wrap**: Create a `MicroArchitectureAnalysis` initialized with this pipeline.
3.  **Solve**: The `WorklistSolver` (from Analysis framework) will use the `MicroArchitectureAnalysis` (which uses your pipeline's `cycle()` and `isReady()` methods) to compute the fixpoint.