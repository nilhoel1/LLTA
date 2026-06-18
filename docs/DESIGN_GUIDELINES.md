# LLTA — Design Guidelines

Keep the **generic, reusable layer** (analysis framework, program graph, pass
skeleton, ILP) free of target-specific knowledge. Everything that depends on a
particular device — instruction latencies, instruction validation, disassembly
quirks, memory model, target options — lives under `Targets/<arch>/` behind the
`RTTarget` interface.

## Layering (lower may not depend on higher)

```
Graph                      ProgramGraph (target-agnostic)
Analysis                   abstract interpretation, cache, pipeline framework
Targets                    RTTarget + registry + per-device code (MSP430, ...)
ILP                        abstract solvers + backend selection
MIRPasses                  generic passes + pipeline builder
tool (LLTA.cpp)            wiring
```

`TimingAnalysisBase` (the results container) deliberately does **not** depend on
`Targets` — the registry is invoked by the pipeline builder, and the resolved
`RTTarget` is stored back into the container. Don't reintroduce that edge.

## Adding a new target

1. Add `include/Targets/<Arch>/<Name>Target.h` + `lib/Targets/<Arch>/<Name>Target.cpp`
   subclassing `llta::RTTarget`. Use a **family base + device** split when one
   ISA has several devices (see MSP430: `MSP430Target` family →
   `MSP430FR5994Target` device). Put ISA-level behavior on the family base and
   device specifics (memory model, clock, caches, options) on the device.
2. Implement the hooks: `getInstructionLatency`, `checkInstruction`,
   `getMaxInstructionWords`, `isControlFlowMnemonic`, `resolveBranchTarget`, and
   (optionally) `getMemoryModelPasses`.
3. Register it in `lib/Targets/TargetRegistry.cpp`: map the `Triple::ArchType`
   to a family, and resolve the concrete device (default to the device you
   implement).
4. Add the sources to `lib/Targets/CMakeLists.txt`. Target-specific CLI options
   go in a `<Name>Options.{h,cpp}` in the target dir, not in `Utility/Options`.
5. No generic pass should `switch` on the arch — query `TAR.getTarget()` instead.

A target may also be **data-only** (empirical model + assumptions, no code yet),
parked for a future implementation — see `lib/Targets/ESP32-C6/`.

## Testing

- **Regression first.** Any change to timing must keep `tests/regression_test.py`
  green: it analyzes the MSP430 benchmarks and checks the WCET against known
  baselines (`cnt` = 6347, `cover` = 3483). Run a build + this script after every
  behavior-affecting change; a refactor must leave the WCET **unchanged**.
- **Unit tests** for self-contained components live in `tests/unit/`
  (e.g. `CacheModuleTests.cpp`) and run via CTest. Add unit tests for new generic
  framework pieces (cache policies, graph utilities, solvers).
- WCET is an **upper bound**: when converting a solver's floating-point objective
  to cycles, round to the nearest integer (FP noise) — never truncate, which
  would under-report and is unsound.

## Conventions

- C++ with LLVM style: `CamelCase` types, `CamelCase` locals/members, `clang-format`
  (`.clang-format`) and `clang-tidy` (`.clang-tidy`) are configured — run them.
- Generic LLTA code is in `namespace llvm`; the target layer is in `namespace llta`
  (so `RTTarget`/`TargetRegistry` don't collide with `llvm::Target`/`llvm::TargetRegistry`).
- Match the comment density and idioms of the surrounding file.
