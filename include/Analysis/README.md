# Analysis Framework

This directory contains the **Abstract Analysis Framework** for WCET analysis using Abstract Interpretation.

## Overview

The framework provides interfaces and base classes for implementing transfer function-based analyses that are composed together and solved using a fixpoint algorithm.

## Core Interfaces

| Class                | Purpose                                    |
| -------------------- | ------------------------------------------ |
| `AbstractState`      | Lattice element (state at a program point) |
| `AbstractAnalysable` | Transfer function interface                |
| `PipelineAnalysis`   | Composite analysis orchestrator            |
| `WorklistSolver`     | Fixpoint solver                            |

## AbstractState

Represents the abstract state at a program point. Must implement:
- `clone()` - Deep copy the state
- `equals(Other)` - Check equality with another state
- `join(Other)` - Join (merge) with another state, return true if changed
- `toString()` - Debug representation

## AbstractAnalysable

Interface for any analysis component.
- `getInitialState()` - Return the starting state
- `process(State, MI)` - Apply transfer function for instruction `MI`, return cycle cost

## Assumptions & Limitations

> **⚠️ Abstract Interpretation Limitations**
> 
> This framework uses **Abstract Interpretation**. This means:
> 1. **Over-approximation**: Results are safe but potentially pessimistic.
> 2. **State Explosion**: Some micro-architectures (e.g., out-of-order CPUs) may have state spaces too large to analyze precisely.
> 3. **In-order Pipelines**: The `MicroArchitectureAnalysis` wrapper assumes an in-order pipeline model. Out-of-order execution requires more sophisticated state representations.
> 4. **No Speculation**: Branch prediction effects must be modeled separately and composed.

## Extending the Framework

To add a new analysis:

1. **Define a State Class** (inheriting `AbstractState`)
2. **Implement an Analysable** (inheriting `AbstractAnalysable`)
3. **Add to the Pipeline** (via `PipelineAnalysis::addAnalysis`)

See `MicroArchitectureAnalysis.h` for an example using the Hardware Pipeline model.
