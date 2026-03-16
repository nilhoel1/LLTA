# Tech Stack

## Core Technologies
- **LLVM Version**: 20.1.8 (Source-based build)
- **Language**: C++ (Standard inferred from LLVM: C++17/20)
- **Build System**: CMake, Ninja (preferred) or Make

## External Dependencies
- **Gurobi**: Optional, for ILP solving in PathAnalysis.
- **HiGHS**: Optional, open-source alternative for ILP solving.

## Toolchain components
- **Compiler**: `clang` / `clang++` (preferred) or System `cc`/`c++`
- **Linker**: `lld` (preferred) or System `ld`