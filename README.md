# LLTA - Low Level Timing Analysis

LLTA is a WCET (Worst-Case Execution Time) timing-analysis infrastructure built
on LLVM. It operates on the Machine IR (MIR) and runs as a drop-in replacement
for `llc`, injecting analysis passes into the code-generation pipeline. The
architecture under analysis is identified by the LLVM target triple.

## Documentation

- [`docs/OVERVIEW.md`](docs/OVERVIEW.md) — directory map, the analysis pass
  pipeline, and where to find what.
- [`docs/DESIGN_GUIDELINES.md`](docs/DESIGN_GUIDELINES.md) — layering rules,
  testing policy, and how to add a target.
- [`docs/TargetAssumptions.md`](docs/TargetAssumptions.md) — what the design
  assumes of every target.
- [`CLAUDE.md`](CLAUDE.md) — quick guidance for working in the codebase.

Target-specific code lives under `lib/Targets/<arch>/` + `include/Targets/<arch>/`
(MSP430 is the implemented target; `lib/Targets/ESP32-C6/` is a parked,
data-only model). The reusable layer is `Graph/`, `Analysis/`, `MIRPasses/`,
`ILP/`, `Pipeline/`, `Utility/`.

## Prerequisites

- **LLVM 20.1.8** — auto-downloaded by the build scripts.
- **ILP solver** — Gurobi (commercial) or HiGHS (open source); either is fine.
- **Clang**, **CMake**, **Ninja**, **curl & tar**.

## Quick start

```bash
./config.sh config      # download LLVM (first time), apply patches, configure
./config.sh build       # build the `llta` tool (+ LoopBoundPlugin)
python3 tests/regression_test.py   # WCET regression on the MSP430 benchmarks
```

`config.sh` commands: `download`, `patch`, `config`, `build`, `build-all`,
`clean`, `mrproper`.

## Usage

`llta` takes an LLVM IR file (`.ll`/`.bc`), like `llc`:

```bash
./build/bin/llta <input.ll> [options]
```

Common options:

- `-march=<arch>` — target architecture (e.g. `msp430`).
- `-start-function=<name>` — analysis entry point (default `main`).
- `-dump-file=<path>` — objdump disassembly (`objdump -Dl`) of the linked ELF;
  used by `AdressResolverPass` to recover real instruction addresses, static
  jump/call targets, and data objects.
- `-ilp-solver=<auto|gurobi|highs>` — abstract ILP backend (`auto` prefers
  Gurobi if licensed, else HiGHS).
- `-loop-bounds-json=<path>` — loop bounds from the clang plugin.

MSP430(FR) target options (owned by the MSP430 target): `-fram-start=<hex>`,
`-fram-wait-states=<n>`, `-fram-cache`, `-fram-cache-policy`,
`-fram-cache-sets/-ways/-line-bytes`, `-fram-cache-verbose`. These are no-ops
unless set, so default runs are unaffected. See `docs/OVERVIEW.md` and
`--help` for the full list.

### Preparing input

For loop-bound detection, the IR must be canonical (rotated loops for SCEV):

```bash
opt -passes='mem2reg,instcombine,loop-simplify,loop-rotate,indvars' input.ll -S -o input_opt.ll
```

For loops SCEV can't bound, annotate in C with the `LoopBoundPlugin`:

```c
#pragma loop_bound(1, 10)
for (int i = 0; i < n; i++) { ... }
```

See [`clang-plugin/README.md`](clang-plugin/README.md).

## Manual build (advanced)

```bash
CC=clang CXX=clang++ cmake \
    -S externalDeps/llvm-project-20.1.8.src/llvm -B build \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD='MSP430;RISCV' \
    -DLLVM_EXTERNAL_LLTA_SOURCE_DIR=./LLTA \
    -DLLVM_EXTERNAL_CLANG_PLUGIN_SOURCE_DIR=./clang-plugin \
    -DLLVM_EXTERNAL_PROJECTS='LLTA;clang-plugin' \
    -DLLVM_ENABLE_PROJECTS='clang' \
    -DCLANG_SOURCE_DIR=externalDeps/llvm-project-20.1.8.src/clang \
    -DLLVM_USE_LINKER=lld -GNinja
cd build && ninja llta LoopBoundPlugin
```

## Updating to a new LLVM version

LLTA is based on LLVM's `llc` and built as an external LLVM project, with custom
LLVM changes carried in a patch.

1. Bump `LLVM_VERSION` in `scripts/download_llvm.sh`,
   `scripts/create_llvm_patch.sh`, and `config.sh`.
2. `./scripts/download_llvm.sh` to fetch + extract the new source.
3. Keep a pristine copy for patch generation:
   `cp -r externalDeps/llvm-project-<v>.src externalDeps/llvm-project-<v>.src.original`.
4. Re-base the tool on the new `llc`: copy `llc.cpp` → `LLTA.cpp` and
   `NewPMDriver.{cpp,h}`, then re-apply the LLTA modifications (pass injection,
   CLI options, headers/initialization, pass registration) — use the old patch
   as a guide.
5. Apply any other LLVM-side changes from the old patch; resolve conflicts.
6. `./scripts/create_llvm_patch.sh create` to regenerate
   `scripts/llvm-<v>-custom.patch`.
7. `./config.sh config && ./config.sh build` and verify the regression suite.
8. Update version references here and in the CMake files.

Patch helpers: `./scripts/create_llvm_patch.sh {create|apply}` diff/apply against
the `.original` tree.

## Supported targets

- **MSP430** (device: MSP430FR5994) — implemented.
- **ESP32-C6** (RISC-V RV32IMAC) — empirical model parked in
  `lib/Targets/ESP32-C6/`; not yet implemented.

## Testing

`python3 tests/regression_test.py` analyzes the MSP430 benchmarks and checks the
WCET against known baselines. Unit tests for generic components are under
`tests/unit/` (CTest). A refactor must leave the WCET unchanged.
