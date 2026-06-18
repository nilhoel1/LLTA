# Development Plan

## 1. Memory Analysis

### 1.1 ToDos

- [x] Add MSP430 Toolchain , to build examples here.
- [x] Extract static jump/call targets and data/heap objects (name/address/size/section) from the dump in `AdressResolverPass`, stored on `TimingAnalysisResults` (foundation; no consumer yet).
- [x] Double Check completenes of `AdressResolverPass`. (Coverage self-report under
  `-address-resolver-verbose`: 100% of analysed-function instructions addressed on
  both the in-tree cases and the msp430-freertos target, 0 re-sync/mismatch/leftover.)
- [x] Link LLVM IR analyses with custom LLTA analyses. (`FRAMWaitStatePass` consumes
  the resolved `InstructionAddressMap` + `FRAMStart` and adds MSP430 FRAM fetch
  wait-states into the WCET latency path; gated by `-fram-wait-states`.)

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