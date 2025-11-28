# Examples Directory

This directory contains examples and tests for LLTA features.

## Structure

- **`optimizer_tests/`**: Examples demonstrating optional optimizer integration
  - Gurobi optional example
  - HiGHS optional example
  - Combined build and test script

## Optimizer Tests

The optimizer tests demonstrate how to build LLTA with optional support for different optimization libraries.

### Running the Tests

```bash
cd optimizer_tests
./build_example.sh
```

This will:
1. Build and test Gurobi example with/without Gurobi
2. Build and test HiGHS example with/without HiGHS
3. Report test results

### Building Individual Examples

```bash
cd optimizer_tests
mkdir build
cd build

# Build with both optimizers
cmake .. -DENABLE_GUROBI=ON -DENABLE_HIGHS=ON
make

# Build without optimizers
cmake .. -DENABLE_GUROBI=OFF -DENABLE_HIGHS=OFF
make
```

### Example Code Pattern

The examples show the proper pattern for optional optimizer integration:

```c
#ifdef ENABLE_GUROBI
#include "gurobi_c.h"
#endif

int solve_problem(double *result) {
#ifdef ENABLE_GUROBI
    // Gurobi implementation
    return solve_with_gurobi(result);
#else
    // Fallback implementation
    return simple_heuristic(result);
#endif
}
```

This same pattern is used throughout LLTA to support optional optimizer backends.
