# LoopBoundPlugin - Clang Plugin for Loop Bound Annotations

## Overview

The LoopBoundPlugin is a Clang plugin that parses custom pragma annotations for loop bounds in C source code. This plugin is designed to work with the LLTA timing analysis infrastructure for Worst-Case Execution Time (WCET) analysis.

## Features

- Parses `#pragma loop_bound(lower, upper)` annotations
- Associates bounds with corresponding loop statements
- Supports for, while, and do-while loops
- Optional verbose output for debugging (silent by default)

## Pragma Syntax

```c
#pragma loop_bound(lower, upper)
for (int i = 0; i < n; i++) {
    // loop body
}
```

Where `lower` is the minimum number of iterations and `upper` is the maximum number of iterations for the loop immediately following the pragma.

### Examples

```c
// Simple loop with bounds [1, 10]
#pragma loop_bound(1, 10)
for (int i = 0; i < 10; i++) { }

// Nested loops
#pragma loop_bound(1, 30)
for (int i = 0; i < rows; i++) {
    #pragma loop_bound(1, 40)
    for (int j = 0; j < cols; j++) { }
}

// While loop that executes at most 100 times
#pragma loop_bound(0, 100)
while (condition) { }

// Do-while loop
#pragma loop_bound(1, 50)
do { } while (condition);
```

## Building

The plugin is built automatically as part of the LLTA project when LLVM is configured with `BUILD_SHARED_LIBS=ON`:

```bash
cmake -S llvm -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_SHARED_LIBS=ON \
  -DLLVM_ENABLE_RTTI=ON \
  -DLLVM_TARGETS_TO_BUILD='MSP430' \
  -DLLVM_EXTERNAL_LLTA_SOURCE_DIR=./LLTA \
  -DLLVM_EXTERNAL_PROJECTS='LLTA' \
  -DLLVM_ENABLE_PROJECTS='clang' \
  -GNinja

ninja LoopBoundPlugin
```

The plugin will be built as `build/lib/LoopBoundPlugin.dylib` (macOS) or `build/lib/LoopBoundPlugin.so` (Linux).

## Usage

### Basic Usage (Silent Mode - Default)

```bash
clang -cc1 -triple msp430 \
  -load ./build/lib/LoopBoundPlugin.dylib \
  -plugin loop-bound \
  your_file.c
```

### With Verbose Output

```bash
clang -cc1 -triple msp430 \
  -load ./build/lib/LoopBoundPlugin.dylib \
  -plugin loop-bound \
  -plugin-arg-loop-bound verbose \
  your_file.c
```

Alternatively, use `-plugin-arg-loop-bound -v` for verbose output.

### Help

```bash
clang -cc1 -triple msp430 \
  -load ./build/lib/LoopBoundPlugin.dylib \
  -plugin loop-bound \
  -plugin-arg-loop-bound help
```

## Testing

Run the automated test script to verify the plugin works correctly:

```bash
./LLTA/clang-plugin/test_plugin.sh
```

See `LLTA/examples/loop_bound_example.c` for a complete example:
```

The test script will:

- Verify the plugin loads correctly
- Test pragma parsing with example files
- Validate loop bound association
- Test silent and verbose modes
- Verify help message
- **Test JSON file generation**
- **Validate JSON content structure and values**
- **Automatically clean up generated files (.json, .ll, .o, .S)**
- Report pass/fail status for each test

All generated files are automatically cleaned up after testing, leaving only source `.c` files in the examples directory.

## Example

See `IR2MIR/examples/loop_bound_example.c` for a complete example:

```c
#include <stdint.h>

#define ARRAY_SIZE 30
#define INNER_SIZE 40

void Initialize(int16_t ArrayA[ARRAY_SIZE][INNER_SIZE]) {
    #pragma loop_bound(1, 30)
    for (int OuterIndex = 0; OuterIndex < ARRAY_SIZE; OuterIndex++) {
        #pragma loop_bound(1, 40)
        for (int InnerIndex = 0; InnerIndex < INNER_SIZE; InnerIndex++) {
            ArrayA[OuterIndex][InnerIndex] = (int16_t)OuterIndex;
        }
    }
}
```

## Implementation Notes

- The plugin uses a global map to store loop bounds during preprocessing
- Bounds are associated with loops by comparing source location offsets
- The plugin searches within 200 bytes before a loop statement for a matching pragma
- Each pragma is consumed after being associated with a loop
- By default, the plugin runs silently (no debug output)
- Verbose mode can be enabled with `-plugin-arg-loop-bound verbose` or `-plugin-arg-loop-bound -v`

## Requirements

- LLVM/Clang built with `BUILD_SHARED_LIBS=ON`
- RTTI enabled (`LLVM_ENABLE_RTTI=ON`)
- Target architecture (e.g., MSP430)

## Troubleshooting

### Plugin doesn't load

- Ensure LLVM was built with shared libraries (`BUILD_SHARED_LIBS=ON`)
- Verify the plugin path is correct
- Check that RTTI is enabled

### Pragmas not recognized

- Ensure the pragma is placed directly before the loop
- Check pragma syntax: `#pragma loop_bound(lower, upper)`
- Use verbose mode to see what the plugin is processing

### No output

- By default, the plugin runs silently
- Add `-plugin-arg-loop-bound verbose` to see debug output
- Errors are still reported through Clang's diagnostic system

## Integration with WCET Analysis

The plugin parses and validates loop bounds at the source level. Future work will integrate the plugin output with the `MachineLoopBoundAgregatorPass` to provide loop iteration counts for WCET path analysis when ScalarEvolution cannot compute constant trip counts.

## Files

- `LoopBoundPlugin.cpp` - Main plugin implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - This file
- `test_plugin.sh` - Automated test script

## Future Enhancements

- Emit LLVM IR metadata for loop bounds
- Support for nested pragma scopes
- Integration with loop optimization passes
- Export bounds to external configuration files
- Variable bounds and symbolic expressions
