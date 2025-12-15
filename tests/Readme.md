# LLTA Tests

This directory contains test cases and benchmarks for the LLTA project.

## Generating Loop Bounds

To generate loop bound JSON files from C source files using the LoopBoundPlugin, use the `generate_loop_bounds.sh` script.

### Usage

```bash
./generate_loop_bounds.sh <arch> <target>
```

Example:

```bash
./generate_loop_bounds.sh msp430 cnt
```

## Preparing LLVM IR for SCEV Analysis

The `MachineLoopBoundAgregatorPass` relies on LLVM's ScalarEvolution (SCEV) analysis to automatically detect loop bounds. For SCEV to work effectively, the input LLVM IR must be in a canonical form. Unoptimized IR (generated with `-O0`) often uses memory for loop counters and lacks the structure SCEV expects.

To ensure SCEV can detect loop bounds, you must optimize the IR. The critical passes are `mem2reg` (to promote stack variables to registers) and `loop-rotate` (to put loops in do-while form).

### Recommended Optimization Command

Use `opt` to run the necessary passes:

```bash
opt -passes='mem2reg,instcombine,loop-simplify,loop-rotate,indvars' input.ll -S -o input_opt.ll
```

Alternatively, `-O1` usually provides sufficient optimization:

```bash
opt -O1 input.ll -S -o input_opt.ll
```

If SCEV returns `COULDNOTCOMPUTE`, it is likely because the loop is not in a rotated canonical form.

## Running MSP430 Tests

The `tests/msp430` directory contains a modular Makefile for building and analyzing benchmarks (e.g., from `tests/srcMaelardalen`).

### Prerequisites

Download the MSP430 toolchain and support files:

```bash
make -C tests/msp430 download
```

### Building a Test Case

To build a test case (e.g., `cnt`) from source to ELF binary and dump:

```bash
make -C tests/msp430 TEST=cnt all
```

This will:
1. Compile `cnt.c` to LLVM IR.
2. Optimize the IR (`-passes='mem2reg,instcombine,loop-simplify,loop-rotate,indvars'`).
3. Run LLTA in compiler-driver mode (`-llc`) to perform transformations (like Call Splitting) and generate assembly.
4. Sanitize the assembly (remove incompatible debug directives).
5. Assemble and link using the MSP430 GCC toolchain.

Artifacts are generated in `tests/msp430/build_cnt/`.

### Running WCET Analysis

To run the full LLTA WCET analysis:

```bash
make -C tests/msp430 TEST=cnt analyze
```

This will:
1. Generate a loop bounds JSON file using the Clang `LoopBoundPlugin`.
2. Run `llta` on the optimized IR, using the loop bounds and the binary dump for address resolution.
3. Output the analysis report to `build_cnt/cnt.wcet`.
