# Pipeline Modeling Framework

This directory contains the **Hardware Pipeline Modeling** framework for cycle-accurate simulation of processor pipelines.

## Overview

The `HardwarePipeline` class models a sequence of pipeline stages (Fetch, Decode, Execute, etc.) and simulates instruction flow through them cycle-by-cycle.

## Core Components

| Class                   | Description                              |
| ----------------------- | ---------------------------------------- |
| `AbstractHardwareStage` | Interface for a pipeline stage           |
| `HardwarePipeline`      | Container for stages, handles simulation |

## AbstractHardwareStage Interface

```cpp
class AbstractHardwareStage {
public:
  virtual void cycle() = 0;              // Advance by one cycle
  virtual bool isReady() const = 0;      // Can accept new instruction?
  virtual void execute(const MachineInstr *MI) = 0;  // Receive instruction
  virtual unsigned getBusyCycles() const = 0;  // For fast-forwarding
  virtual bool isEmpty() const = 0;
  virtual const MachineInstr *getCurrentInstruction() const = 0;
  virtual std::unique_ptr<AbstractHardwareStage> clone() const = 0;
};
```

## Adding a New Pipeline Stage

### Step 1: Inherit from AbstractHardwareStage

```cpp
#include "Pipeline/HardwarePipeline.h"

class MyExecuteStage : public llvm::AbstractHardwareStage {
  const llvm::MachineInstr *CurrentInst = nullptr;
  unsigned BusyCycles = 0;
  
public:
  void cycle() override {
    if (BusyCycles > 0) --BusyCycles;
    if (BusyCycles == 0) CurrentInst = nullptr;
  }
  
  bool isReady() const override { return !CurrentInst; }
  
  void execute(const llvm::MachineInstr *MI) override {
    CurrentInst = MI;
    BusyCycles = 3; // Example: 3-cycle stage
  }
  
  unsigned getBusyCycles() const override { return BusyCycles; }
  bool isEmpty() const override { return !CurrentInst; }
  const llvm::MachineInstr *getCurrentInstruction() const override { return CurrentInst; }
  std::unique_ptr<llvm::AbstractHardwareStage> clone() const override {
    return std::make_unique<MyExecuteStage>(*this);
  }
};
```

### Step 2: Assemble the Pipeline

```cpp
llvm::HardwarePipeline buildPipeline() {
  llvm::HardwarePipeline P;
  P.addStage(std::make_unique<FetchStage>());
  P.addStage(std::make_unique<DecodeStage>());
  P.addStage(std::make_unique<MyExecuteStage>());
  return P;
}
```

### Step 3: Integrate with Analysis

Use `MicroArchitectureAnalysis` (in `Analysis/`) to wrap the pipeline as an `AbstractAnalysable`.

## Future Extensibility

- **Flushing**: Add `flush()` method for branch misprediction handling.
- **Shared Resources**: Add `ResourceManager` for bus arbitration.
