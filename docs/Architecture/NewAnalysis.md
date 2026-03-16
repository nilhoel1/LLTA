# LLTA Analysis Approaches: Legacy vs. Abstract Interpretation

This document provides a technical deep-dive into the two WCET analysis approaches implemented in LLTA: the **Legacy Analysis** (ProgramGraph-based) and the **Abstract Analysis** (Abstract Interpretation framework). Both approaches produce identical WCET results but use fundamentally different algorithms and data structures.

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Legacy Analysis (ProgramGraph)](#legacy-analysis-programgraph)
3. [Abstract Analysis Framework](#abstract-analysis-framework)
4. [Detailed Technical Comparison](#detailed-technical-comparison)
5. [Integration in PathAnalysisPass](#integration-in-pathanalysispass)
6. [Extending the Framework](#extending-the-framework)

---

## Executive Summary

### Quick Comparison

| Aspect | Legacy Analysis | Abstract Analysis |
|--------|----------------|-------------------|
| **Primary Class** | `ProgramGraph` | `AbstractStateGraph` |
| **State Model** | `MuArchState` (min/max cycles) | `AbstractState` (lattice-based) |
| **Computation** | Direct ILP solve | Worklist fixpoint → ILP solve |
| **Solver Interface** | `ILPSolver` | `AbstractILPSolver` |
| **Implementations** | `GurobiSolver`, `HighsSolver` | `AbstractGurobiSolver`, `AbstractHighsSolver` |
| **Algorithm Type** | Static graph analysis | Abstract interpretation |
| **Extensibility** | Monolithic | Composable via `AbstractAnalysable` |
| **Code Location** | `lib/RTTargets/ProgramGraph.cpp` | `lib/Analysis/` + `lib/Pipeline/` |

### When to Use Each Approach

**Legacy Analysis** (`--ilp-solver=gurobi` or `--ilp-solver=highs`):
- Simple WCET computation on static control flow
- When you need direct ILP formulation
- For debugging graph construction issues
- Baseline verification

**Abstract Analysis** (`--ilp-solver=auto`):
- When extending with custom analyses (pipeline simulation, cache analysis)
- When you need compositional analysis components
- For research in abstract interpretation techniques
- When building microarchitectural models

**Verification Mode** (`--ilp-solver=all`):
- Runs all 4 solver configurations
- Compares results for consistency
- Useful for testing and validation

---

## Legacy Analysis (ProgramGraph)

### Architecture Overview

The Legacy Analysis represents the original LLTA approach, inherited from LLVMTA. It builds a static control flow graph where each node contains a microarchitectural state with simple cycle bounds.

```cpp
// From include/RTTargets/ProgramGraph.h:19-30
struct MuArchState {
  unsigned MinCycles;
  unsigned MaxCycles;
  std::string DebugInfo;

  MuArchState(unsigned Min, unsigned Max, std::string Info = "")
      : MinCycles(Min), MaxCycles(Max), DebugInfo(Info) {}
  virtual ~MuArchState() = default;

  unsigned getUpperBoundCycles() const { return MaxCycles; }
  unsigned getLowerBoundCycles() const { return MinCycles; }
};
```

### Core Components

#### 1. ProgramGraph Node Structure

```cpp
// From include/RTTargets/ProgramGraph.h:32-113
class Node {
public:
  explicit Node(unsigned NewId, std::unique_ptr<MuArchState> State);

  // Graph connectivity
  const std::set<unsigned> getPredecessors() const;
  const std::set<unsigned> getSuccessors() const;
  void addSuccessor(unsigned SuccessorId);
  void addPredecessor(unsigned PrededecessorId);

  // State access
  MuArchState &getState() const;

  // Loop properties
  bool IsLoop = false;
  bool IsNestedLoop = false;
  unsigned int LowerLoopBound;
  unsigned int UpperLoopBound;
  Node *NestedLoopHeader;

private:
  unsigned Id;
  StringRef Name;
  std::set<unsigned> Successors;
  std::set<unsigned> Predecessors;
  std::set<unsigned> BackEdgePredecessors;
  std::unique_ptr<MuArchState> State;
};
```

**Key Design Decisions:**
- Uses `std::unique_ptr<MuArchState>` for memory management
- Stores loop bounds directly in nodes
- Simple predecessor/successor sets for CFG edges
- Back-edge tracking for loop identification

#### 2. ProgramGraph Container

```cpp
// From include/RTTargets/ProgramGraph.h:115-230
class ProgramGraph {
public:
  ProgramGraph();

  // Graph construction
  unsigned addNode(std::unique_ptr<MuArchState> State, MachineBasicBlock *MBB);
  void addEdge(unsigned FromNode, unsigned ToNode);

  // Graph queries
  const std::map<unsigned, Node> &getNodes() const;
  const std::set<unsigned> getPredecessors(unsigned NodeId) const;
  const std::set<unsigned> getSuccessors(unsigned NodeId) const;
  bool hasEdge(unsigned FromNode, unsigned ToNode) const;

  // Function-level mapping
  std::map<const MachineBasicBlock *, unsigned> MBBToNodeMap;
  std::map<const Function *, unsigned> FunctionToEntryNodeMap;
  std::map<const Function *, std::vector<unsigned>> FunctionToReturnNodesMap;
  std::vector<std::pair<unsigned, const Function *>> CallSites;

  // Filling from LLVM IR
  bool fillGraphWithFunction(
      MachineFunction &MF, bool IsEntry,
      const std::unordered_map<const MachineBasicBlock *, unsigned int> &MBBLatencyMap,
      const std::unordered_map<const MachineBasicBlock *, unsigned int> &LoopBoundMap = {},
      MachineLoopInfo *MLI = nullptr);

private:
  std::map<unsigned, Node> Nodes;
  unsigned NextNodeId;
};
```

**Key Design Decisions:**
- Uses `std::map<unsigned, Node>` for ordered node storage
- Separate mappings for MBB, function entries, and call sites
- Supports both inter-procedural and intra-procedural analysis
- Direct filling from `MachineFunction` with latency information

#### 3. ILPSolver Interface

```cpp
// From include/ILP/ILPSolver.h:11-44
struct ILPResult {
  bool Success;
  double ObjectiveValue;
  std::map<unsigned, double> NodeExecutionCounts;
  std::map<std::pair<unsigned, unsigned>, double> EdgeExecutionCounts;
  std::string StatusMessage;
};

class ILPSolver {
public:
  virtual ~ILPSolver() = default;

  virtual ILPResult solveWCET(
      const ProgramGraph &MASG,
      unsigned EntryNodeId,
      unsigned ExitNodeId,
      const std::map<unsigned, unsigned> &LoopBoundMap) = 0;

  virtual std::string getName() const = 0;
  virtual bool isAvailable() const = 0;
};
```

**ILP Formulation:**
The Legacy Analysis formulates the WCET problem as an ILP with:

1. **Decision Variables**:
   - `x_i`: Execution count for node i
   - `y_{i,j}`: Execution count for edge (i→j)

2. **Objective**: Maximize Σ(x_i × cost_i)

3. **Constraints**:
   - Flow conservation: Σ(in_edges) = Σ(out_edges)
   - Loop bounds: x_header ≤ loop_bound
   - Entry: x_entry = 1
   - Exit: x_exit = 1

### Algorithm Flow

```
MachineFunction
       │
       ▼
┌──────────────────┐
│ FillMuGraphPass  │  Build ProgramGraph from MIR
└──────────────────┘
       │
       ▼
┌──────────────────┐
│ InstructionLatency │ Assign cycles to MBBs
└──────────────────┘
       │
       ▼
┌──────────────────┐
│ LoopBoundAggregator │ Collect loop bounds
└──────────────────┘
       │
       ▼
┌──────────────────┐
│   ILPSolver      │  Formulate & solve ILP
│  (Gurobi/HiGHS)  │
└──────────────────┘
       │
       ▼
   WCET Result
```

### Example: GurobiSolver Implementation

The `GurobiSolver` (in `lib/ILP/GurobiSolver.cpp`) directly constructs the ILP model:

```cpp
// Simplified illustration
ILPResult GurobiSolver::solveWCET(
    const ProgramGraph &MASG,
    unsigned EntryNodeId,
    unsigned ExitNodeId,
    const std::map<unsigned, unsigned> &LoopBoundMap) {

  // Create GRBEnv and GRBModel
  GRBEnv env = GRBEnv(true);
  GRBModel model = GRBModel(env);

  // Add decision variables for each node
  std::map<unsigned, GRBVar> NodeVars;
  for (const auto &NodePair : MASG.getNodes()) {
    unsigned Id = NodePair.first;
    NodeVars[Id] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS);
  }

  // Set objective: maximize Σ(x_i * cost_i)
  GRBLinExpr obj = 0.0;
  for (const auto &NodePair : MASG.getNodes()) {
    unsigned Id = NodePair.first;
    const Node &N = NodePair.second;
    double cost = N.getState().getUpperBoundCycles();
    obj += NodeVars[Id] * cost;
  }
  model.setObjective(obj, GRB_MAXIMIZE);

  // Add flow conservation constraints
  // ...

  // Add loop bound constraints
  for (const auto &[LoopHeader, Bound] : LoopBoundMap) {
    model.addConstr(NodeVars[LoopHeader] <= Bound);
  }

  // Solve
  model.optimize();

  // Extract results
  ILPResult Result;
  Result.ObjectiveValue = model.get(GRB_DoubleAttr_ObjVal);
  // ...

  return Result;
}
```

---

## Abstract Analysis Framework

The Abstract Analysis framework introduces a formal abstract interpretation approach with a lattice-based state model and worklist fixpoint computation. This enables compositional analyses and easier extension.

### Architecture Overview

```cpp
// Core abstract state interface (include/Analysis/AbstractState.h:14-38)
class AbstractState {
public:
  virtual ~AbstractState() = default;

  // Lattice operations
  virtual std::unique_ptr<AbstractState> clone() const = 0;
  virtual bool equals(const AbstractState *Other) const = 0;
  virtual bool join(const AbstractState *Other) = 0;

  // Debug representation
  virtual std::string toString() const = 0;
};
```

### Core Components

#### 1. AbstractState (Lattice Element)

Every analysis must implement the `AbstractState` interface:

```cpp
// Example: Pipeline state for microarchitectural simulation
class PipelineState : public AbstractState {
public:
  std::vector<std::unique_ptr<AbstractHardwareStage>> Stages;

  // Join operation: take maximum of busy cycles for each stage
  bool join(const AbstractState *Other) override {
    auto *O = static_cast<const PipelineState *>(Other);
    bool Changed = false;

    for (size_t i = 0; i < Stages.size(); ++i) {
      unsigned OldBusy = Stages[i]->getBusyCycles();
      unsigned OtherBusy = O->Stages[i]->getBusyCycles();
      Stages[i]->setBusyCycles(std::max(OldBusy, OtherBusy));
      Changed |= (Stages[i]->getBusyCycles() != OldBusy);
    }

    return Changed;
  }

  std::unique_ptr<AbstractState> clone() const override {
    auto NewState = std::make_unique<PipelineState>();
    for (const auto &Stage : Stages) {
      NewState->Stages.push_back(Stage->clone());
    }
    return NewState;
  }

  bool equals(const AbstractState *Other) const override {
    auto *O = static_cast<const PipelineState *>(Other);
    if (Stages.size() != O->Stages.size()) return false;

    for (size_t i = 0; i < Stages.size(); ++i) {
      if (!Stages[i]->equals(O->Stages[i].get())) return false;
    }
    return true;
  }

  std::string toString() const override {
    return "PipelineState"; // Simplified
  }
};
```

#### 2. AbstractAnalysable (Transfer Functions)

```cpp
// From include/Analysis/AbstractAnalysable.h:14-28
class AbstractAnalysable {
public:
  virtual ~AbstractAnalysable() = default;

  // Create initial abstract state
  virtual std::unique_ptr<AbstractState> getInitialState() = 0;

  // Transfer function: process instruction and return cost
  virtual unsigned process(AbstractState *State, const MachineInstr *MI) = 0;
};
```

**Example: Microarchitectural Pipeline Analysis**

```cpp
// From include/RTTargets/MSP430/MSP430Pipeline.h
class MSP430Pipeline : public AbstractAnalysable {
public:
  std::unique_ptr<AbstractState> getInitialState() override {
    auto State = std::make_unique<MicroArchState>();
    // Initialize pipeline stages (Fetch, Decode, Execute, etc.)
    State->Pipeline.addStage(std::make_unique<FetchStage>());
    State->Pipeline.addStage(std::make_unique<DecodeStage>());
    State->Pipeline.addStage(std::make_unique<ExecuteStage>());
    return State;
  }

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    auto *S = static_cast<MicroArchState *>(State);

    // Inject instruction into pipeline
    S->Pipeline.injectInstruction(MI);

    // Simulate until instruction retires
    unsigned TotalCycles = 0;
    while (!S->Pipeline.isRetired()) {
      S->Pipeline.cycle();
      TotalCycles++;

      // Fast-forward optimization
      if (S->Pipeline.canFastForward()) {
        unsigned FastForwardCycles = S->Pipeline.convertCyclesToFastForward();
        TotalCycles += FastForwardCycles;
      }
    }

    return TotalCycles;
  }
};
```

#### 3. AbstractStateGraph

```cpp
// From include/Analysis/AbstractStateGraph.h:15-69
class AbstractStateGraph {
public:
  struct Node {
    unsigned Id;
    std::unique_ptr<AbstractState> State;
    const MachineBasicBlock *MBB;
    bool IsEntry;
    bool IsExit;
    bool IsLoopHeader;
    unsigned UpperLoopBound;
    unsigned Cost;  // Computed cost for this node

    Node(unsigned Id, std::unique_ptr<AbstractState> State,
         const MachineBasicBlock *MBB = nullptr)
        : Id(Id), State(std::move(State)), MBB(MBB),
          IsEntry(false), IsExit(false), IsLoopHeader(false),
          UpperLoopBound(0), Cost(0) {}
  };

  struct Edge {
    unsigned To;
    bool IsBackEdge;
    bool operator<(const Edge &Other) const { return To < Other.To; }
  };

  // Graph construction
  unsigned addNode(std::unique_ptr<AbstractState> State,
                   const MachineBasicBlock *MBB = nullptr);
  void addEdge(unsigned From, unsigned To, bool IsBackEdge = false);

  // Graph queries
  Node *getNode(unsigned Id);
  const std::map<unsigned, std::unique_ptr<Node>> &getNodes() const;
  const std::set<Edge> &getSuccessors(unsigned Id) const;
  const std::set<unsigned> &getPredecessors(unsigned Id) const;

  // Call graph information
  std::map<const Function *, unsigned> FunctionEntries;
  std::map<const Function *, std::vector<unsigned>> FunctionReturns;
  struct CallSite {
    unsigned CallNodeId;
    unsigned ReturnNodeId;
    const Function *Callee;
  };
  std::vector<CallSite> CallSites;

private:
  unsigned NextNodeId;
  std::map<unsigned, std::unique_ptr<Node>> Nodes;
  std::map<unsigned, std::set<Edge>> AdjacencyList;
  std::map<unsigned, std::set<unsigned>> Predecessors;
};
```

**Key Differences from ProgramGraph:**
- Uses `AbstractState` instead of `MuArchState`
- Pre-computed `Cost` per node (from analysis)
- Back-edge flag in edge structure
- Similar call graph tracking

#### 4. WorklistSolver (Fixpoint Algorithm)

```cpp
// From include/Analysis/WorklistSolver.h:25-57
class WorklistSolver {
public:
  WorklistSolver(AbstractAnalysable &Analysis, AbstractStateGraph &Graph)
      : Analysis(Analysis), Graph(Graph) {}

  // Run analysis on pre-built ProgramGraph
  void run(const ProgramGraph &PG);

  // Run analysis on MachineFunction directly
  void run(MachineFunction &MF, MachineLoopInfo *MLI = nullptr,
           const std::map<const MachineBasicBlock *, unsigned> *LoopBounds = nullptr);

  const AbstractStateGraph &getGraph() const { return Graph; }

private:
  AbstractAnalysable &Analysis;
  AbstractStateGraph &Graph;
  std::deque<unsigned> Worklist;
  std::set<unsigned> InWorklist;

  void addToWorklist(unsigned NodeId);
  unsigned takeFromWorklist();
  void initializeGraph(const ProgramGraph &PG);
};
```

**Fixpoint Algorithm:**

```cpp
// Simplified WorklistSolver algorithm (lib/Analysis/WorklistSolver.cpp)
void WorklistSolver::run(const ProgramGraph &PG) {
  // Initialize AbstractStateGraph from ProgramGraph
  initializeGraph(PG);

  // Add all nodes to worklist
  for (const auto &[Id, Node] : Graph.getNodes()) {
    addToWorklist(Id);
  }

  // Worklist iteration until fixpoint
  while (!Worklist.empty()) {
    unsigned Id = takeFromWorklist();

    // Get current state
    AbstractStateGraph::Node *CurrentNode = Graph.getNode(Id);
    std::unique_ptr<AbstractState> CurrentState = CurrentNode->State->clone();

    // Join predecessor states
    for (unsigned PredId : Graph.getPredecessors(Id)) {
      AbstractStateGraph::Node *PredNode = Graph.getNode(PredId);
      CurrentState->join(PredNode->State.get());
    }

    // Check if state changed
    if (!CurrentState->equals(CurrentNode->State.get())) {
      CurrentNode->State = std::move(CurrentState);

      // Process instructions and compute cost
      unsigned Cost = 0;
      if (CurrentNode->MBB) {
        for (const MachineInstr &MI : *CurrentNode->MBB) {
          Cost += Analysis.process(CurrentNode->State.get(), &MI);
        }
      }
      CurrentNode->Cost = Cost;

      // Add successors to worklist
      for (const auto &Edge : Graph.getSuccessors(Id)) {
        if (!Edge.IsBackEdge) {  // Don't propagate through back-edges
          addToWorklist(Edge.To);
        }
      }
    }
  }
}
```

**Algorithm Properties:**
- **Convergence**: Guaranteed by lattice properties (monotonic join)
- **Complexity**: O(N × E) in worst case, where N = nodes, E = edges
- **Optimization**: Worklist avoids reprocessing unchanged nodes
- **Precision**: Can be more precise than static analysis through iterative refinement

#### 5. AbstractILPSolver Interface

```cpp
// From include/ILP/AbstractILPSolver.h:10-26
struct AbstractILPResult {
  double WCET;
  std::vector<unsigned> WorstCasePath;  // Node IDs
  std::map<unsigned, double> ExecutionCounts;
};

class AbstractILPSolver {
public:
  virtual ~AbstractILPSolver() = default;

  // Solve WCET on pre-computed AbstractStateGraph
  virtual AbstractILPResult solveWCET(const AbstractStateGraph &ASG) = 0;
};
```

**Simplified ILP Formulation:**

The Abstract Analysis formulates a simpler ILP because costs are pre-computed:

1. **Decision Variables**:
   - `x_i`: Execution count for node i (already analyzed)

2. **Objective**: Maximize Σ(x_i × precomputed_cost_i)

3. **Constraints**:
   - Flow conservation (simplified, no state variables)
   - Loop bounds
   - Entry/Exit constraints

**Example: AbstractGurobiSolver**

```cpp
// Simplified illustration (lib/ILP/AbstractGurobiSolver.cpp)
AbstractILPResult AbstractGurobiSolver::solveWCET(
    const AbstractStateGraph &ASG) {

  GRBModel model = GRBModel(env);

  // Add decision variables (fewer than Legacy due to pre-computation)
  std::map<unsigned, GRBVar> NodeVars;
  for (const auto &[Id, NodePtr] : ASG.getNodes()) {
    NodeVars[Id] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS);
  }

  // Set objective using pre-computed costs
  GRBLinExpr obj = 0.0;
  for (const auto &[Id, NodePtr] : ASG.getNodes()) {
    obj += NodeVars[Id] * NodePtr->Cost;  // Cost already computed!
  }
  model.setObjective(obj, GRB_MAXIMIZE);

  // Add simplified constraints
  // ...

  model.optimize();

  AbstractILPResult Result;
  Result.WCET = model.get(GRB_DoubleAttr_ObjVal);
  return Result;
}
```

### Algorithm Flow

```
MachineFunction
       │
       ▼
┌──────────────────┐
│ FillMuGraphPass  │  Build ProgramGraph
└──────────────────┘
       │
       ▼
┌──────────────────┐
│ WorklistSolver   │  Convert to AbstractStateGraph
│  + Pipeline      │  Run fixpoint analysis
└──────────────────┘
       │
       ▼
┌──────────────────┐
│ AbstractILPSolver│  Solve simplified ILP
│ (Gurobi/HiGHS)   │
└──────────────────┘
       │
       ▼
   WCET Result
```

---

## Detailed Technical Comparison

### 1. State Modeling Philosophy

#### Legacy Analysis: Static Min/Max

```cpp
// Simple interval-based state
struct MuArchState {
  unsigned MinCycles;  // Lower bound
  unsigned MaxCycles;  // Upper bound
};
```

**Characteristics:**
- **Representation**: Single interval [MinCycles, MaxCycles]
- **Computation**: Direct assignment from instruction latencies
- **Precision**: May over-estimate due to lack of context sensitivity
- **Complexity**: O(1) per node

#### Abstract Analysis: Lattice-based

```cpp
// Lattice element with join operation
class AbstractState {
  virtual bool join(const AbstractState *Other) = 0;
  virtual bool equals(const AbstractState *Other) = 0;
};
```

**Characteristics:**
- **Representation**: Arbitrary complex state (pipeline, cache, etc.)
- **Computation**: Iterative refinement via fixpoint algorithm
- **Precision**: Can be more precise through join operations
- **Complexity**: O(N × E) for fixpoint computation

**Example: Pipeline State Join**

```cpp
// Legacy: Simple max
unsigned LegacyCost = std::max(CycleCount1, CycleCount2);

// Abstract: Per-stage max
bool PipelineState::join(const AbstractState *Other) {
  auto *O = static_cast<const PipelineState *>(Other);

  for (size_t i = 0; i < Stages.size(); ++i) {
    unsigned OldBusy = Stages[i]->getBusyCycles();
    unsigned OtherBusy = O->Stages[i]->getBusyCycles();
    Stages[i]->setBusyCycles(std::max(OldBusy, OtherBusy));
    // ...
  }
  // Return whether state changed
}
```

### 2. Algorithm Comparison

#### Legacy Analysis: Direct ILP

```
Step 1: Build ProgramGraph (O(N))
  - Create nodes with MuArchState
  - Add edges from CFG
  - Store loop bounds

Step 2: Formulate ILP (O(N + E))
  - Create variables for nodes/edges
  - Add flow conservation constraints
  - Add loop bound constraints

Step 3: Solve ILP (NP-hard, handled by Gurobi/HiGHS)
  - Branch-and-bound or simplex
  - Return optimal WCET

Total: O(N + E) + ILP solve time
```

#### Abstract Analysis: Fixpoint + ILP

```
Step 1: Build ProgramGraph (O(N))
  - Same as Legacy

Step 2: Run WorklistSolver (O(N × E) worst case)
  - Initialize AbstractStateGraph
  - Iteratively join predecessor states
  - Process instructions through transfer functions
  - Compute per-node costs

Step 3: Solve simplified ILP (fewer variables/constraints)
  - Use pre-computed costs
  - Simpler constraints

Total: O(N + E + N×E) + ILP solve time
```

**Practical Performance:**
- Legacy: Faster graph building, slower ILP (more variables)
- Abstract: Slower graph building (fixpoint), faster ILP (pre-computed)
- For MSP430: Abstract often faster due to small graph size

### 3. Solver Implementation Differences

#### Legacy ILPSolver Interface

```cpp
// Takes raw graph and external loop bounds
ILPResult solveWCET(
    const ProgramGraph &MASG,
    unsigned EntryNodeId,
    unsigned ExitNodeId,
    const std::map<unsigned, unsigned> &LoopBoundMap  // External!
);
```

**Responsibilities:**
- Extract state information from nodes
- Build full ILP model from scratch
- Handle loop bounds externally
- Return both node and edge execution counts

#### AbstractILPSolver Interface

```cpp
// Takes pre-analyzed graph with embedded loop bounds
AbstractILPResult solveWCET(
    const AbstractStateGraph &ASG  // Already analyzed!
);
```

**Responsibilities:**
- Use pre-computed node costs
- Build simplified ILP model
- Loop bounds embedded in graph nodes
- Return WCET and worst-case path

**Variable Count Comparison:**

| Component | Legacy | Abstract |
|-----------|--------|----------|
| Node variables | O(N) | O(N) |
| Edge variables | O(E) | 0 (flow only) |
| State variables | O(N × state_size) | 0 (pre-computed) |
| Loop constraints | O(L) | 0 (embedded) |
| **Total** | **O(N + E + N×S)** | **O(N)** |

### 4. Memory Footprint

#### Legacy Analysis

```cpp
// Per-node memory
struct Node {
  std::unique_ptr<MuArchState> State;  // 2 × unsigned = 8 bytes
  std::set<unsigned> Predecessors;     // ~O(log N) per edge
  std::set<unsigned> Successors;       // ~O(log N) per edge
  StringRef Name;
  bool IsLoop;
  unsigned int LowerLoopBound;
  unsigned int UpperLoopBound;
  Node *NestedLoopHeader;
};

// Total: ~32 + O(log N) bytes per node
```

#### Abstract Analysis

```cpp
// Per-node memory (AbstractStateGraph)
struct Node {
  std::unique_ptr<AbstractState> State;  // Variable size
  const MachineBasicBlock *MBB;
  unsigned Id;
  bool IsEntry;
  bool IsExit;
  bool IsLoopHeader;
  unsigned UpperLoopBound;
  unsigned Cost;  // Pre-computed!
};

// Total: ~32 + sizeof(AbstractState) bytes per node
// AbstractState can be large (pipeline stages, cache lines, etc.)
```

**Memory Trade-off:**
- Legacy: Lower per-node memory, higher solver memory
- Abstract: Higher per-node memory (complex states), lower solver memory

### 5. Extensibility Mechanisms

#### Legacy Analysis: Monolithic

To extend Legacy Analysis with a new analysis (e.g., cache simulation):

```cpp
// Must modify MuArchState
struct ExtendedMuArchState : public MuArchState {
  std::map<unsigned, bool> CacheState;  // Add cache lines

  // Must modify ProgramGraph
  // Must modify ILPSolver to handle new variables
  // Must modify constraint generation
};
```

**Drawbacks:**
- Tightly coupled components
- Changes propagate across multiple files
- Hard to compose multiple analyses

#### Abstract Analysis: Composable

To extend Abstract Analysis with a new analysis:

```cpp
// 1. Define new state
class CacheState : public AbstractState {
  std::map<unsigned, bool> CacheLines;

  bool join(const AbstractState *Other) override {
    // Implement cache join (e.g., union of misses)
  }
  // ...
};

// 2. Define new analysis component
class CacheAnalysis : public AbstractAnalysable {
  std::unique_ptr<AbstractState> getInitialState() override {
    return std::make_unique<CacheState>();
  }

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    // Simulate cache access
    return CacheMissCycles;
  }
};

// 3. Compose with existing pipeline analysis
class CombinedAnalysis : public AbstractAnalysable {
  std::vector<std::unique_ptr<AbstractAnalysable>> Components;

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    unsigned TotalCycles = 0;
    for (auto &Component : Components) {
      TotalCycles += Component->process(State, MI);
    }
    return TotalCycles;
  }
};
```

**Benefits:**
- Loosely coupled via interfaces
- Easy to compose multiple analyses
- Changes localized to new component

### 6. Precision and Accuracy

Both approaches should produce **identical WCET results** for the MSP430 target when properly configured. Verification mode (`--ilp-solver=all`) ensures this:

```
=== Unified Solver Comparison Table ===
+----------+-----------+------------+---------+-------------+----------------+
| Type     | Solver    | Available  | Success | WCET (cyc)  | Time (ms)      |
+----------+-----------+------------+---------+-------------+----------------+
| Legacy   | Gurobi    | Yes        | Yes     |        6347 |          1.468 |
| Legacy   | HiGHS     | Yes        | Yes     |        6347 |          1.013 |
| Abstract | Gurobi    | Yes        | Yes     |        6347 |          0.938 |
| Abstract | HiGHS     | Yes        | Yes     |        6347 |          0.654 |
+----------+-----------+------------+---------+-------------+----------------+
```

**Why do they match?**
- Both use the same underlying cycle latencies (InstructionLatencyPass)
- Both enforce the same loop bounds
- Both solve the same fundamental WCET problem
- Abstract fixpoint converges to the same state as static analysis

**When can they differ?**
- Bugs in implementation (should not happen)
- Different loop bound interpretations
- Numerical precision issues in solvers

---

## Integration in PathAnalysisPass

Both analysis approaches are integrated in `PathAnalysisPass::doFinalization` (lib/MIRPasses/PathAnalysisPass.cpp:89-471).

### Solver Selection

```cpp
// From lib/MIRPasses/PathAnalysisPass.cpp:93
ILPSolverType SolverType = parseILPSolverType(ILPSolverOption);
```

**Command-line options:**
- `--ilp-solver=auto`: Use Abstract Analysis (preferred)
- `--ilp-solver=gurobi`: Legacy with Gurobi
- `--ilp-solver=highs`: Legacy with HiGHS
- `--ilp-solver=all`: Run all 4 configurations

### Execution Flow

#### Single Solver Mode

```cpp
// From lib/MIRPasses/PathAnalysisPass.cpp:176-223
if (SolverType == ILPSolverType::All) {
  // Special handling
} else {
  // Single solver mode
  auto Solver = createILPSolver(SolverType);

  outs() << "Using ILP solver: " << Solver->getName() << "\n";
  Result = Solver->solveWCET(TAR.MASG, EntryNodeId, ExitNodeId,
                             EmptyLoopBoundMap);

  outs() << "WCET: " << static_cast<unsigned>(Result.ObjectiveValue)
         << " cycles\n";
}
```

#### Abstract Analysis Verification

```cpp
// From lib/MIRPasses/PathAnalysisPass.cpp:226-468
// --- Abstract Analysis Verification & Comparison ---
outs() << "\n=== Abstract Analysis Verification ===\n";

// Run Abstract Analysis on pre-built ProgramGraph
AnalysisWorker.run(TAR.MASG);

// Compare results
if (SolverType == ILPSolverType::All) {
  // Run all 4 solver configurations
  // Print unified comparison table
} else {
  // Single solver comparison
  auto AbstractSolver = ...;
  auto AbstractResult = AbstractSolver->solveWCET(AnalysisWorker.getGraph());

  double Diff = std::abs(AbstractResult.WCET - Result.ObjectiveValue);
  if (Diff < 1e-6) {
    outs() << "[SUCCESS] WCET matches!\n";
  } else {
    outs() << "[DIFFERENCE] WCET differs by " << Diff << " cycles\n";
  }
}
```

### Key Integration Points

1. **Shared Data Structures**:
   - Both use `TAR.MASG` (ProgramGraph) built by `FillMuGraphPass`
   - Loop bounds from `MachineLoopBoundAgregatorPass`
   - Instruction latencies from `InstructionLatencyPass`

2. **Abstract Analysis Initialization**:
   ```cpp
   // PathAnalysisPass.h:35-38
   AbstractStateGraph ASG;
   MSP430Pipeline Pipeline;
   WorklistSolver AnalysisWorker;  // Wraps Pipeline and ASG
   ```

3. **Result Comparison**:
   - Legacy returns `ILPResult` with node/edge counts
   - Abstract returns `AbstractILPResult` with WCET and path
   - Both compared in unified table for `--ilp-solver=all`

---

## Extending the Framework

This section provides practical guidance for extending LLTA with new analyses using the Abstract Interpretation framework.

### Example: Adding Cache Analysis

#### Step 1: Define Cache State

```cpp
// include/Analysis/CacheState.h
#ifndef CACHE_STATE_H
#define CACHE_STATE_H

#include "Analysis/AbstractState.h"
#include <map>

namespace llvm {

class CacheState : public AbstractState {
public:
  // Map cache line to "in cache" (true) or "not in cache" (false)
  std::map<unsigned, bool> CacheLines;

  CacheState() {
    // Initialize: all cache lines empty
    for (unsigned i = 0; i < 16; ++i) {  // 16-line cache
      CacheLines[i] = false;
    }
  }

  // Join: union of cache misses (pessimistic)
  bool join(const AbstractState *Other) override {
    auto *O = static_cast<const CacheState *>(Other);
    bool Changed = false;

    for (unsigned i = 0; i < 16; ++i) {
      // If either state misses, result is miss
      bool OldMiss = !CacheLines[i];
      bool OtherMiss = !O->CacheLines[i];
      CacheLines[i] = !(OldMiss || OtherMiss);  // Only in cache if both in cache
      Changed |= (!CacheLines[i] != OldMiss);
    }

    return Changed;
  }

  std::unique_ptr<AbstractState> clone() const override {
    auto NewState = std::make_unique<CacheState>();
    NewState->CacheLines = CacheLines;
    return NewState;
  }

  bool equals(const AbstractState *Other) const override {
    auto *O = static_cast<const CacheState *>(Other);
    return CacheLines == O->CacheLines;
  }

  std::string toString() const override {
    return "CacheState";
  }

  // Helper: simulate cache access
  bool access(unsigned Address) {
    unsigned Line = (Address / 16) % 16;  // Direct-mapped, 16-byte lines

    if (CacheLines[Line]) {
      return true;  // Cache hit
    } else {
      CacheLines[Line] = true;
      return false;  // Cache miss
    }
  }
};

} // namespace llvm
#endif
```

#### Step 2: Define Cache Analysis Component

```cpp
// include/Analysis/CacheAnalysis.h
#ifndef CACHE_ANALYSIS_H
#define CACHE_ANALYSIS_H

#include "Analysis/AbstractAnalysable.h"
#include "Analysis/CacheState.h"
#include "llvm/CodeGen/MachineInstr.h"

namespace llvm {

class CacheAnalysis : public AbstractAnalysable {
public:
  unsigned CacheMissPenalty = 10;  // Cycles per cache miss

  std::unique_ptr<AbstractState> getInitialState() override {
    return std::make_unique<CacheState>();
  }

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    auto *CS = static_cast<CacheState *>(State);

    // Check if instruction accesses memory
    if (MI->mayLoad() || MI->mayStore()) {
      // Extract address (simplified)
      unsigned Addr = extractAddress(MI);

      // Access cache
      bool Hit = CS->access(Addr);

      // Return cost: 0 for hit, penalty for miss
      return Hit ? 0 : CacheMissPenalty;
    }

    return 0;  // No memory access
  }

private:
  unsigned extractAddress(const MachineInstr *MI) {
    // Simplified: extract from operand
    // Real implementation would decode addressing mode
    if (MI->getNumOperands() > 0) {
      return MI->getOperand(0).getImm();
    }
    return 0;
  }
};

} // namespace llvm
#endif
```

#### Step 3: Compose with Pipeline Analysis

```cpp
// include/Analysis/CombinedAnalysis.h
#ifndef COMBINED_ANALYSIS_H
#define COMBINED_ANALYSIS_H

#include "Analysis/AbstractAnalysable.h"
#include "RTTargets/MSP430/MSP430Pipeline.h"
#include "Analysis/CacheAnalysis.h"
#include <memory>
#include <vector>

namespace llvm {

class CombinedAnalysis : public AbstractAnalysable {
public:
  std::unique_ptr<AbstractAnalysable> Pipeline;
  std::unique_ptr<AbstractAnalysable> Cache;

  CombinedAnalysis() {
    Pipeline = std::make_unique<MSP430Pipeline>();
    Cache = std::make_unique<CacheAnalysis();
  }

  // Need a combined state
  std::unique_ptr<AbstractState> getInitialState() override {
    // This requires a CompositeState that holds multiple states
    auto State = std::make_unique<CompositeState>();
    State->States.push_back(Pipeline->getInitialState());
    State->States.push_back(Cache->getInitialState());
    return State;
  }

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    auto *CS = static_cast<CompositeState *>(State);

    unsigned TotalCycles = 0;
    TotalCycles += Pipeline->process(CS->States[0].get(), MI);
    TotalCycles += Cache->process(CS->States[1].get(), MI);

    return TotalCycles;
  }
};

} // namespace llvm
#endif
```

#### Step 4: Register in PathAnalysisPass

```cpp
// In PathAnalysisPass.h:36-38
// Change:
// MSP430Pipeline Pipeline;

// To:
// CombinedAnalysis Pipeline;  // Now includes cache!

// Or make it configurable:
// std::unique_ptr<AbstractAnalysable> Analysis;
```

### Example: Adding a New Target Architecture

When adding a new architecture (e.g., RISC-V), you can leverage the Abstract Analysis framework:

#### Step 1: Define Target-Specific Pipeline

```cpp
// include/RTTargets/RISC-V/RISCVPipeline.h
namespace llvm {

class RISCVPipeline : public AbstractAnalysable {
public:
  std::unique_ptr<AbstractState> getInitialState() override {
    auto State = std::make_unique<MicroArchState>();

    // RISC-V 5-stage pipeline
    State->Pipeline.addStage(std::make_unique<FetchStage>());
    State->Pipeline.addStage(std::make_unique<DecodeStage>());
    State->Pipeline.addStage(std::make_unique<ExecuteStage>());
    State->Pipeline.addStage(std::make_unique<MemoryStage>());
    State->Pipeline.addStage(std::make_unique<WriteBackStage>());

    return State;
  }

  unsigned process(AbstractState *State, const MachineInstr *MI) override {
    // Similar to MSP430, but with RISC-V opcodes
    auto *S = static_cast<MicroArchState *>(State);
    S->Pipeline.injectInstruction(MI);

    unsigned TotalCycles = 0;
    while (!S->Pipeline.isRetired()) {
      S->Pipeline.cycle();
      TotalCycles++;
    }

    return TotalCycles;
  }
};

} // namespace llvm
```

#### Step 2: Select Pipeline Based on Target

```cpp
// In PathAnalysisPass constructor (PathAnalysisPass.cpp:44)
PathAnalysisPass::PathAnalysisPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {

  // Select pipeline based on target
  std::string Target = TAR.getTargetTriple();

  if (Target.contains("msp430")) {
    AnalysisWorker = WorklistSolver(std::make_unique<MSP430Pipeline>(), ASG);
  } else if (Target.contains("riscv")) {
    AnalysisWorker = WorklistSolver(std::make_unique<RISCVPipeline>(), ASG);
  } else {
    report_error("Unsupported target");
  }
}
```

### Migration Path from Legacy to Abstract

If you have existing code using Legacy Analysis and want to migrate:

1. **Identify Custom MuArchState Subclasses**:
   ```cpp
   // Legacy code
   struct CustomState : public MuArchState {
     int CustomField;
   };
   ```

2. **Convert to AbstractState**:
   ```cpp
   // New code
   class CustomAbstractState : public AbstractState {
     int CustomField;

     bool join(const AbstractState *Other) override {
       auto *O = static_cast<const CustomAbstractState *>(Other);
       int OldVal = CustomField;
       CustomField = std::max(CustomField, O->CustomField);
       return CustomField != OldVal;
     }

     // Implement clone, equals, toString...
   };
   ```

3. **Create Transfer Functions**:
   ```cpp
   class CustomAnalysis : public AbstractAnalysable {
     // Implement getInitialState and process...
   };
   ```

4. **Register with PathAnalysisPass**:
   ```cpp
   // In PathAnalysisPass.h
   std::unique_ptr<AbstractAnalysable> CustomAnalysis;
   ```

---

## Conclusion

LLTA's dual analysis architecture provides both a proven legacy approach and a modern abstract interpretation framework. Key takeaways:

### Design Trade-offs

| Aspect | Legacy | Abstract |
|--------|--------|----------|
| **Simplicity** | ✓ Simpler data structures | ✗ More complex framework |
| **Performance** | ✓ Faster graph building | ✗ Slower fixpoint iteration |
| **Extensibility** | ✗ Monolithic, hard to extend | ✓ Composable, modular |
| **Precision** | May over-estimate | Can be more precise |
| **Maintainability** | ✗ Tightly coupled | ✓ Loosely coupled via interfaces |
| **Learning Curve** | ✓ Easier to understand | ✗ Requires abstract interpretation knowledge |

### Recommendation

**For MSP430 WCET Analysis:**
- Use **Abstract Analysis** (`--ilp-solver=auto`) as default
- More extensible for future enhancements
- Better performance for typical programs
- Foundation for microarchitectural modeling

**For Legacy Compatibility:**
- Keep Legacy Analysis for regression testing
- Use `--ilp-solver=all` for verification
- Maintain both implementations for consistency checks

**For Research & Extensions:**
- Build on **Abstract Analysis** framework
- Implement custom `AbstractState` and `AbstractAnalysable`
- Leverage composability for complex analyses

Both approaches are maintained and tested, ensuring LLTA remains a robust platform for WCET analysis research and development.
