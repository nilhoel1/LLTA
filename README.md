# LLTA - Low Level Timing Analysis

LLTA is a timing analysis infrastructure built on top of LLVM. It operates on the Machine Intermediate Representation (MIR) to perform Worst-Case Execution Time (WCET) analysis. It functions as a drop-in replacement for `llc` (LLVM Static Compiler) with additional analysis passes injected into the code generation pipeline.

## Project Structure

The project is organized as follows:

- **`LLTA.cpp`**: The main driver tool, based on LLVM's `llc`.
- **`lib/MIRPasses/`**: Contains the core analysis passes that operate on MIR.
- **`lib/RTTargets/`**: Contains target-specific micro-architectural models (currently MSP430).
- **`lib/Utility/`**: Helper utilities and options.
- **`clang-plugin/`**: A Clang plugin (`LoopBoundPlugin`) for annotating loop bounds in C source code.
- **`tests/`**: Benchmarks and test scripts.
- **`scripts/`**: Utility scripts for downloading LLVM and managing patches.
- **`externalDeps/`**: Downloaded LLVM source code.

## Prerequisites

- **LLVM 20.1.8**: Automatically downloaded by the build scripts.
- **Gurobi Optimizer**: Required for the Path Analysis pass (ILP solving).
- **Clang**: System clang for building.
- **CMake & Ninja**: Build system.
- **curl & tar**: For downloading LLVM source.

## Quick Start

### 1. Download and Configure

From the LLTA workspace root:

```bash
./config.sh config
```

This will:

- Download LLVM 20.1.8 source if not present
- Apply any custom patches if available
- Configure the build with CMake

### 2. Build

```bash
./config.sh build
```

Or build everything including clang:

```bash
./config.sh build-all
```

## Build Script Commands

The `config.sh` script provides several commands:

```bash
./config.sh download    # Download LLVM 20.1.8 source
./config.sh patch       # Apply custom patches
./config.sh config      # Configure build (auto-downloads if needed)
./config.sh build       # Build LLTA target only
./config.sh build-all   # Build all targets (LLTA, clang, etc.)
```

## Management Scripts

### Download LLVM Source

Located at `scripts/download_llvm.sh`:

```bash
./scripts/download_llvm.sh
```

Downloads LLVM 20.1.8 source tarball from GitHub releases and extracts it to `externalDeps/llvm-project-20.1.8.src/`.

### Patch Management

Located at `scripts/create_llvm_patch.sh`:

**Create a patch** from modifications:
```bash
./scripts/create_llvm_patch.sh create
```

Compares `externalDeps/llvm-project-20.1.8.src.original/` (base) with `externalDeps/llvm-project-20.1.8.src/` (modified) and generates `scripts/llvm-20.1.8-custom.patch`.

**Apply a patch**:
```bash
./scripts/create_llvm_patch.sh apply
```

Applies the patch file to the modified LLVM source.

## Updating to a New LLVM Version

When updating LLTA to work with a new LLVM version, follow these steps:

### 1. Update Version Configuration

Edit the following files and update the `LLVM_VERSION` variable:

- `scripts/download_llvm.sh`
- `scripts/create_llvm_patch.sh`
- `config.sh`

Example:

```bash
LLVM_VERSION="20.2.0"  # Update to new version
```

### 2. Download New LLVM Source

```bash
./scripts/download_llvm.sh
```

This downloads and extracts the new LLVM version to `externalDeps/llvm-project-<version>.src/`.

### 3. Create a Backup (Original) Copy

```bash
cp -r externalDeps/llvm-project-<version>.src externalDeps/llvm-project-<version>.src.original
```

This creates the base version for patch generation.

### 4. Copy LLTA Base (llc)

LLTA is based on LLVM's `llc` tool. Copy the new version:

```bash
# Copy the new llc tool files
cp externalDeps/llvm-project-<version>.src/llvm/tools/llc/llc.cpp LLTA.cpp

# If there's a NewPMDriver, copy it too
cp externalDeps/llvm-project-<version>.src/llvm/tools/llc/NewPMDriver.cpp NewPMDriver.cpp
cp externalDeps/llvm-project-<version>.src/llvm/tools/llc/NewPMDriver.h NewPMDriver.h
```

### 5. Apply LLTA Modifications

Manually re-apply the LLTA-specific modifications to the copied files:

- Integrate analysis passes into the MIR pipeline
- Add custom command-line options
- Include LLTA headers and initialization code
- Register custom passes

Refer to existing modifications or the old patch file as a guide.

### 6. Apply Additional LLVM Modifications

If the existing patch file has modifications to other LLVM files (beyond llc), apply those to the new LLVM source:

```bash
# If you have an old patch, try applying it (may need manual fixes)
cd externalDeps/llvm-project-<version>.src
patch -p1 < ../../scripts/llvm-<old-version>-custom.patch
```

Resolve any conflicts manually.

### 7. Generate New Patch File

Once all modifications are complete:

```bash
./scripts/create_llvm_patch.sh create
```

This generates `scripts/llvm-<version>-custom.patch` containing all differences between the original and modified LLVM source.

### 8. Test the Build

```bash
./config.sh config
./config.sh build
```

Verify that LLTA builds and runs correctly with the new LLVM version.

### 9. Update Documentation

Update version references in:

- This README.md
- CMakeLists.txt files
- Any version-specific documentation

## Detailed Build Instructions

### Manual Configuration (Advanced)

If you need to configure manually or customize the build:

```bash
CC=clang CXX=clang++ cmake \
    -S externalDeps/llvm-project-20.1.8.src/llvm \
    -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_TARGETS_TO_BUILD='MSP430' \
    -DLLVM_EXTERNAL_LLTA_SOURCE_DIR=./LLTA \
    -DLLVM_EXTERNAL_CLANG_PLUGIN_SOURCE_DIR=./clang-plugin \
    -DLLVM_EXTERNAL_PROJECTS='LLTA;clang-plugin' \
    -DLLVM_ENABLE_PROJECTS='clang' \
    -DCLANG_SOURCE_DIR=externalDeps/llvm-project-20.1.8.src/clang \
    -DLLVM_USE_LINKER=lld \
    -GNinja
```

### Manual Build

```bash
cd build
ninja llta
```

To build the Clang plugin as well (required for tests):

```bash
ninja LoopBoundPlugin
```

## Usage

### 1. Running LLTA

`llta` is used similarly to `llc`. It takes an LLVM IR file (`.ll` or `.bc`) as input.

```bash
./build/bin/llta <input.ll> [options]
```

**Common Options:**

- `-march=<arch>`: Specify target architecture (e.g., `msp430`).
- `-dump-file=<path>`: Path to dump the ELF file or analysis results.
- `-start-function=<name>`: Entry point for analysis (default: `main`).

### 2. Preparing Input Files

For the analysis to work correctly, especially Loop Bound detection, the input LLVM IR must be in a canonical form.

**Recommended Optimization:**
Use `opt` to prepare your `.ll` files:

```bash
opt -passes='mem2reg,instcombine,loop-simplify,loop-rotate,indvars' input.ll -S -o input_opt.ll
```

*Note: `loop-rotate` is critical for ScalarEvolution (SCEV) to detect loop bounds.*

### 3. Annotating Loop Bounds

For loops where SCEV cannot determine bounds, use the `LoopBoundPlugin` and pragmas in your C code:

```c
#pragma loop_bound(1, 10)
for (int i = 0; i < n; i++) { ... }
```

See `LLTA/clang-plugin/README.md` for details.

## Analysis Passes

LLTA injects several passes into the backend pipeline:

1. **`MachineLoopBoundAgregatorPass`**: Collects loop bounds from SCEV and manual annotations (JSON).
2. **`InstructionLatencyPass`**: Assigns execution latency to each machine instruction based on the target model.
3. **`FillMuGraphPass`**: Constructs a graph representing the micro-architectural state.
4. **`PathAnalysisPass`**: Formulates the WCET problem as an Integer Linear Program (ILP) and solves it using Gurobi.
5. **`AdressResolverPass`**: Resolves symbolic addresses to physical addresses.
6. **`CallSplitterPass`**: Manages context sensitivity by splitting function calls.
7. **`AsmDumpAndCheckPass`**: Verifies the generated assembly against expected output.

## Supported Targets

- **MSP430**: Full support including micro-architectural modeling.

## Testing

Tests are located in `LLTA/tests/`.

To generate loop bounds for a test case:

```bash
./LLTA/tests/generate_loop_bounds.sh msp430 cnt
```

To run a debug session (VS Code):

1. Select "dbg cnt MSP430" in the Run and Debug view.
2. Press F5.
