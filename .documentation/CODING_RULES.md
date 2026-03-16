# Coding Rules

## Style Guide (Inferred)
- **Format**: Follows LLVM coding standards (likely enforced by `clang-format`).
- **Indentation**: 2 spaces.
- **Braces**: Attach to the end of the line (K&R/LLVM style).
- **Naming Conventions**:
    - **Classes/Types**: `CamelCase` (e.g., `PathAnalysisPass`, `TimingAnalysisResults`).
    - **Functions**: `camelCase` (e.g., `runOnMachineFunction`, `getMSP430Latency`) - *Note: LLVM usually uses CamelCase for functions too, but this codebase seems to mix or strictly follow LLVM's variable naming which is CamelCase. Wait, let's double check. `runOnMachineFunction` is mixed. `getMSP430Latency` is camelCase. The file content shows `getMSP430Latency`, `runOnMachineFunction`.*
    - **Variables**: `CamelCase` (e.g., `StartFunctionName`, `CurrentNumReferences`) or `camelCase` for locals.
    - **Member Variables**: Often `CamelCase` (e.g., `DebugPrints`).

## Common Types & Patterns
- **LLVM Types**:
    - `MachineBasicBlock` (MBB): Represents a basic block in MIR.
    - `MachineFunction`: Represents a function.
    - `MachineInstr`: An instruction.
    - `raw_ostream`: Used for output (e.g., `outs()`, `errs()`).
    - `Triple`: Target architecture description.
- **Error Handling**: Use `reportError` or `assert` for invariants.
- **Logging**: Use `outs()` for standard output and debug prints (often guarded by `DebugPrints`), `errs()` for warnings/errors.
