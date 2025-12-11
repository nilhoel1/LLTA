# LLTA Usage Guide

## Quick Start

### Building LLTA
```bash
./config.sh config    # Configure the build
./config.sh build     # Build LLTA
```

### Running WCET Analysis
```bash
# Basic usage
./build/bin/llta path/to/program.ll -march=msp430

# With specific start function
./build/bin/llta program.ll -march=msp430 -start-function=main

# Compare all available solvers
./build/bin/llta program.ll -march=msp430 --ilp-solver=all
```

### Command-Line Options
| Option                     | Description                                           |
| -------------------------- | ----------------------------------------------------- |
| `-march=<arch>`            | Target architecture (e.g., `msp430`)                  |
| `-start-function=<name>`   | Entry function for analysis                           |
| `--ilp-solver=<solver>`    | Solver selection: `auto`, `gurobi`, `highs`, or `all` |
| `-loop-bounds-json=<path>` | JSON file with loop bounds                            |
| `-gIR`                     | Debug mode: dump IR instead of analysis               |

---

## Analysis Framework Architecture

### Two Analysis Paths

LLTA provides two analysis paths that should produce identical results:

1. **Legacy Analysis**: Uses `ProgramGraph` directly with `GurobiSolver`/`HighsSolver`
2. **Abstract Analysis**: Uses `WorklistSolver` to build `AbstractStateGraph`, then solves with `AbstractGurobiSolver`/`AbstractHighsSolver`

When using `--ilp-solver=all`, both paths are executed and compared.

### Analysis Pipeline

```
MachineFunction
      │
      ▼
┌─────────────────────┐
│  CallSplitterPass   │  Split CFG at calls
└─────────────────────┘
      │
      ▼
┌─────────────────────┐
│ InstructionLatency  │  Assign cycle costs (getMSP430Latency)
└─────────────────────┘
      │
      ▼
┌─────────────────────┐
│ MachineLoopBound    │  Collect loop bounds from IR/JSON
│   AgregatorPass     │
└─────────────────────┘
      │
      ▼
┌─────────────────────┐
│   FillMuGraphPass   │  Build ProgramGraph
└─────────────────────┘
      │
      ▼
┌─────────────────────┐
│  PathAnalysisPass   │  Solve WCET via ILP
│   ├── WorklistSolver│  (Abstract Analysis)
│   └── ILP Solvers   │  (Gurobi/HiGHS)
└─────────────────────┘
```

---

## Adding a New Target Architecture

### Step 1: Create Latency Model

Create a new latency function in `lib/MIRPasses/InstructionLatencyPass.cpp`:

```cpp
unsigned getYourArchLatency(const MachineInstr *MI) {
    unsigned Opcode = MI->getOpcode();
    switch (Opcode) {
        // Add your opcode -> cycle mappings
        case YourArch::MOV: return 1;
        case YourArch::ADD: return 1;
        case YourArch::MUL: return 3;
        case YourArch::CALL: return 5;
        // ...
        default: return 1;
    }
}
```

### Step 2: Register in Dispatcher

Add detection logic in `InstructionLatencyPass.cpp`:

```cpp
if (ArchName.contains("yourarch")) {
    return getYourArchLatency(&MI);
}
```

### Step 3: Update Documentation

Update `.ai/PROJECT_MAP.md` and `.ai/ARCHITECTURE.md` to reflect the new architecture.

---

## Adding a New Pipeline/Analysis

The Abstract Analysis framework supports extensible analysis components.

### Step 1: Create a New AbstractAnalysable

Create a header in `include/Analysis/`:

```cpp
// include/Analysis/MyAnalysis.h
#ifndef LLVM_ANALYSIS_MYANALYSIS_H
#define LLVM_ANALYSIS_MYANALYSIS_H

#include "Analysis/AbstractState.h"
#include "Analysis/AbstractAnalysable.h"

namespace llvm {

class MyAnalysisState : public AbstractState {
public:
    // Your state representation
    unsigned SomeValue = 0;
    
    bool join(const AbstractState *Other) override {
        auto *O = static_cast<const MyAnalysisState *>(Other);
        unsigned OldValue = SomeValue;
        SomeValue = std::max(SomeValue, O->SomeValue);
        return SomeValue != OldValue;
    }
    
    std::unique_ptr<AbstractState> clone() const override {
        return std::make_unique<MyAnalysisState>(*this);
    }
    
    bool equals(const AbstractState *Other) const override {
        auto *O = static_cast<const MyAnalysisState *>(Other);
        return SomeValue == O->SomeValue;
    }
};

class MyAnalysis : public AbstractAnalysable {
public:
    std::unique_ptr<AbstractState> getInitialState() override {
        return std::make_unique<MyAnalysisState>();
    }
    
    unsigned process(AbstractState *State, const MachineInstr *MI) override {
        auto *S = static_cast<MyAnalysisState *>(State);
        // Compute transfer function and return cost
        return computeCost(MI);
    }
};

} // namespace llvm
#endif
```

### Step 2: Integrate into PipelineAnalysis

Add your analysis to `PipelineAnalysis` in `lib/Analysis/PipelineAnalysis.cpp`:

```cpp
PipelineAnalysis::PipelineAnalysis() {
    // Register your analysis as a stage
    Stages.push_back(std::make_unique<MyAnalysis>());
}
```

### Step 3: Build and Test

```bash
./config.sh build
python3 tests/regression_test.py
```

---

## Testing

### Regression Tests

```bash
# Run full regression suite
python3 tests/regression_test.py
```

Expected output:
```
=== Summary ===
cnt: GREEN
cover: GREEN  
cnt-all-solvers: GREEN
Final Result: GREEN
```

### Manual Testing

```bash
# Verify solver consistency
./build/bin/llta tests/msp430/cnt/msp.ll -march=msp430 --ilp-solver=all
```

Expected: All 4 solvers (Legacy/Abstract × Gurobi/HiGHS) should produce identical WCET.

---

## Troubleshooting

### "No ILP solver available"
- Ensure Gurobi or HiGHS is installed and properly linked
- Check CMake configuration for `ENABLE_GUROBI` / `ENABLE_HIGHS`

### WCET is 0 cycles
- Check that `MachineBasicBlock`s contain valid instructions
- Verify loop bounds are being detected (check for "Loop node X has bound: Y" in output)

### Solvers produce different results
- This indicates a bug in the analysis
- Use `--ilp-solver=all` to compare and identify which solver differs
- Check loop bound constraints and flow conservation

---

## File Reference

| File                                       | Purpose                                   |
| ------------------------------------------ | ----------------------------------------- |
| `lib/Analysis/WorklistSolver.cpp`          | Worklist-based fixpoint algorithm         |
| `lib/Analysis/AbstractStateGraph.cpp`      | Graph of abstract states                  |
| `lib/Analysis/PipelineAnalysis.cpp`        | Compositional analysis pipeline           |
| `lib/ILP/AbstractGurobiSolver.cpp`         | Solve WCET on AbstractStateGraph (Gurobi) |
| `lib/ILP/AbstractHighsSolver.cpp`          | Solve WCET on AbstractStateGraph (HiGHS)  |
| `lib/MIRPasses/InstructionLatencyPass.cpp` | Architecture-specific latencies           |
| `lib/MIRPasses/PathAnalysisPass.cpp`       | Main WCET analysis orchestration          |
