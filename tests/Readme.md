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
