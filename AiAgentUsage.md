# AI Agent Usage Guide & Workflows

## 1. The Philosophy: "The Context Layer"

This project uses a dedicated **Context Layer** located in the `.ai/` directory. These files are **not** for humans to read linearly, but for AI Agents (**Google Antigravity**, **GitHub Copilot**) to ingest.

By feeding the AI these files, you move it from "Generic C++ Coding Bot" to "Senior LLTA Developer" who understands LLVM passes, MIR, and our specific build system.

## 2. The `.ai` File Structure

Before you start a session, understand what is available:

*   **`MAINTENANCE_PROTOCOL.md`**: The most important file. It forces the AI to update documentation when it writes code.
*   **`ARCHITECTURE.md`**: Explains the MIR pipeline, how `llc` is modified, and the Gurobi solver integration.
*   **`TECH_STACK.md`**: Defines the strict LLVM version (20.1.8), C++ standards, and build tools.
*   **`PROJECT_MAP.md`**: A navigational map of the file tree.
*   **`CODING_RULES.md`**: C++ style guides, memory management (LLVM specific), and naming conventions.

---

## 3. Optimizing for Antigravity & Copilot

### Google Antigravity IDE
Antigravity excels at contextual understanding but requires clear intent. Use explicit file references to "anchor" the agent in the rules.

**Best Practice:**
Start your request by explicitly referencing the key context files:
> "@MAINTENANCE_PROTOCOL.md", "@ARCHITECTURE.md", and "@CODING_RULES.md"

### GitHub Copilot
Copilot thrives on open context and specific slash commands.

**Best Practice:**
1.  Open the relevant `.ai/` files in your tabs (especially `CODING_RULES.md`).
2.  Use `@workspace` to force a broader scan if you are asking high-level questions.
3.  Explicitly command it: "Review the open `MAINTENANCE_PROTOCOL.md` before generating this code."

---

## 4. Master Prompts

Use these prompt templates to ensure high-quality, project-compliant output.

### 📝 Adding & Enhancing Documentation
**Goal:** specific prompt for keeping documentation in sync or improving it.

> **Context:** @MAINTENANCE_PROTOCOL.md @ARCHITECTURE.md @PROJECT_MAP.md
> **Task:** Audit and enhance the project documentation.
> **Instructions:**
> 1. Scan `LLTA.cpp` and `lib/MIRPasses` to determine the *actual* execution order of passes.
> 2. Compare this with the "Analysis Pipeline" section in `ARCHITECTURE.md`. If they differ, update the documentation to match the code.
> 3. Check `PROJECT_MAP.md`. Are there any new files in `lib/` that are missing or have generic "C++ file" descriptions? Update them with accurate 1-sentence summaries of their purpose.
> 4. **Constraint:** Do not modify any C++ code, only the Markdown files.

### 🔬 Adding a New Analysis
**Goal:** specific prompt for correctly implementing a new feature without breaking the build or style guide.

> **Context:** @CODING_RULES.md @ARCHITECTURE.md @MAINTENANCE_PROTOCOL.md
> **Task:** Add a new `CacheAnalysisPass`.
> **Instructions:**
> 1. Create a new MIR pass in `lib/MIRPasses` called `CacheAnalysisPass`.
> 2. It should run **after** `InstructionLatencyPass` but **before** `PathAnalysisPass`.
> 3. The pass should iterate over `MachineBasicBlocks` and access the `TimingAnalysisResults`.
> **Constraints:**
> - Use standard LLVM memory management (no inappropriate `std::shared_ptr`).
> - Register the pass in `LLTA.cpp` (or `NewPMDriver.cpp` depending on the current pipeline setup).
> - **CRITICAL:** You MUST follow the `MAINTENANCE_PROTOCOL.md`:
>    - Update `ARCHITECTURE.md` with the new pipeline order.
>    - Update `PROJECT_MAP.md` with the new files.

---

## 5. Standard Workflows

### Workflow A: Debugging Build Issues
1.  **Context:** "Read `TECH_STACK.md` and `CMakeLists.txt`."
2.  **Error:** Paste the Ninja error output.
3.  **Prompt:** "Based on the `externalDeps` structure defined in the docs, why is CMake failing to find the Gurobi library?"

### Workflow B: Updating LLVM
1.  **Prompt:** "I am updating the `LLVM_VERSION` in `config.sh` to 20.2.0. According to `MAINTENANCE_PROTOCOL.md`, what other files need to be updated? Please generate the text updates for `TECH_STACK.md`."

---

## 6. The "Definition of Done (DoD)"

When the AI says it is finished, **do not trust it immediately**. Verify against the protocol:

1.  **Did `.ai/PROJECT_MAP.md` change?** (If a file was created)
2.  **Did `.ai/ARCHITECTURE.md` change?** (If logic or pipeline changed)
3.  **Did it compile?** (Always run the build check)

If any answer is "No", reject the output:
> "You failed the Maintenance Protocol. Update the .ai documentation files to reflect these code changes."