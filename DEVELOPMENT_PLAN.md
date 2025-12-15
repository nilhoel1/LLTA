# Development Plan

## 1. Memory Analysis

### 1.1 ToDos

- [x] Add MSP430 Toolchain , to build examples here.
- [ ] Double Check completenes of `AdressResolverPass`.
- [ ] Link LLVM IR analyses with custom LLTA analyses.

## 2. ESP32-C6 Pipeline Analysis
Implement a cycle-accurate timing model of the Espressif ESP32-C6 (RISC-V HP Core) within the LLTA framework.

### 2.1 ToDos

- [ ] Create a new RTTarget for the ESP32-C6.
- [ ] Make AdressResolver and memory analysis work for the ESP32-C6.
- [ ] Implement the `AbstractHardwareStage` interface for each pipeline stage.
- [ ] Create a factory function to assemble the `HardwarePipeline`.
- [ ] Wrap the assembled pipeline in `MicroArchitectureAnalysis` to integrate with the Abstract Analysis Framework.
- [ ] Create `lib/RTTargets/ESP32C6/` and register the target in `InstructionLatencyPass.cpp`.
- [ ] Compare LLTA's `CycleCount` prediction against the hardware measurements.