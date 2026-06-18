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
- [x] FRAM read-cache analysis (FR5xx). A modular, reusable cache analysis in
  the abstract-interpretation framework (`include/Analysis/Cache/`):
  `CacheGeometry`, plug-and-play `ReplacementPolicy` modules
  (`UnknownPolicy` — adversarial/sound must default; `LRUPolicy`, `FIFOPolicy` —
  age-based, must+may), a `CacheAccessMapper`, and the generic `CacheAnalysis`
  engine (parameterized by `AnalysisKind` Must/May) driven over the
  `WorklistSolver` CFG fixpoint. `FRAMCacheAnalysisPass` specializes it for
  MSP430 (`FRAMAccessMapper`: FRAM fetch words + SRAM/FRAM data-access barriers):
  the **must-analysis** folds the cache-aware fetch penalty into `MBBLatencyMap`
  (gated by `-fram-cache`, `-fram-cache-policy`, `-fram-cache-{sets,ways,line-bytes}`;
  supersedes `FRAMWaitStatePass`), and under `-fram-cache-verbose` a sound
  **may-analysis** (LRU-may at the real associativity, over-approximating any
  policy) reports `always-miss` accesses (diagnostic only; no WCET change).
  Verified on msp430-freertos: e.g. taskCnt 6329 (no FRAM) ≤ 7708 (cache) ≤ 10227
  (no-cache), 100% resolver coverage retained. The modules have standalone unit
  tests (`tests/unit/CacheModuleTests.cpp`; run via `ctest`/the `check-llta-cache`
  build target).

## 2. ESP32-C6 Pipeline Analysis
Implement a cycle-accurate timing model of the Espressif ESP32-C6 (RISC-V HP Core) within the LLTA framework.

### 2.1 ToDos

- [ ] Create a new RTTarget for the ESP32-C6.
- [ ] Make AdressResolver and memory analysis work for the ESP32-C6.
- [ ] Implement the `AbstractHardwareStage` interface for each pipeline stage.
- [ ] Create a factory function to assemble the `HardwarePipeline`.
- [ ] Wrap the assembled pipeline in `MicroArchitectureAnalysis` to integrate with the Abstract Analysis Framework.
- [ ] Implement `lib/Targets/ESP32-C6/` (an `RTTarget` subclass) and register it in `lib/Targets/TargetRegistry.cpp`. The empirical model + assumptions are already parked there (`ESP32-C6-Model.json`, `results.csv`, `ESP32-C6-Assumptions.md`).
- [ ] Compare LLTA's `CycleCount` prediction against the hardware measurements.