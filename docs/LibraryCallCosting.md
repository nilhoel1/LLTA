# Costing body-less library calls (and why it stops at "UNSOUND-marked")

## The problem

LLTA analyzes the MIR that LLVM produces from the input IR. A `call` whose callee
has **no body in that IR** therefore contributes only the `call` instruction's
own latency — the callee's body cost is silently omitted, making the WCET an
under-approximation. Two flavors occur on MSP430:

- **Source-declared externals** — libm `sin`/`cos`/`atan`/`exp`/`log`/`sqrt`:
  `declare`d in the IR, defined in the linked libm.
- **Backend-synthesized libcalls** — `__mspabi_*` integer mul/div/shift and
  soft-`double`/`float` helpers, emitted during ISel as `MO_ExternalSymbol`
  (never in the IR at all).

`whet` is the extreme case: 6 libm functions + 6 `__mspabi_*` families.

## What LLTA does today (sound by omission-reporting)

LLTA **detects** every reachable body-less call and **reports the WCET as
UNSOUND**, listing the un-costed callees (see `ProgramGraph::finalize` →
`UnsoundExternalCallees`, surfaced by `PathAnalysisPass`). The numeric WCET is
still emitted but explicitly flagged as an under-approximation. The seam
`RTTarget::getExternalCallCost(name)` exists so a target *can* supply a worst-case
body cost (which would both charge it and drop the unsound flag for that callee),
but **no target implements it** — it returns `nullopt`, so everything body-less
stays UNSOUND-marked. This is deliberate: a flagged under-approximation beats an
unflagged wrong number, and beats a sound-but-useless one (below).

## Why we do not compute these costs (the wall)

We investigated computing real worst-case body costs and hit three compounding
obstacles that make a *useful, sound* result impractical for these routines:

1. **LLVM cannot decode the routines.** The benchmarks link gcc `-mlarge`
   libgcc/newlib, so the routines are **MSP430X 20-bit** code (`pushm.a`, `mova`,
   `calla`, `reta`, …). LLVM's MSP430 backend implements only the base 16-bit ISA
   (`FeatureX` is a flag, but there are no MSP430X instruction definitions), so
   its `MCDisassembler` either fails or *mis-decodes* these (verified with
   `llvm-mc`: `mova` → "invalid encoding", `calla` → garbage). So an on-the-fly
   `MCInst`-based analyzer (using the Phase-B latency table) is a non-starter for
   exactly the routines that matter.

2. **The soft-float tree has a statically-unresolvable call.** Costing from GNU
   `objdump` instead (it *does* decode MSP430X — see `scripts/derive_abi_costs.py`)
   gets through the integer routines, but the `double` routines
   (`__mspabi_addd/subd/mpyd/divd`) call newlib `__unpack_d`/`__pack_d`, and
   `__pack_d` contains `call 0(r6)` — an **indirect call through a memory operand**
   (function pointer). No static analysis can soundly bound where it goes, so the
   double routines come back "unresolvable."

3. **Conservative bounds are uselessly loose.** For the routines that *do*
   resolve, a sound conservative bound (loop trip-count capped at 64, a safe
   8 cycles/instruction) over-estimates badly: `__mspabi_mpyi` derives to ~4467
   cycles versus a true worst case of a few hundred (its loop is a fixed 17
   iterations). Charged at whet's ~94 call sites inside 65536-iteration loops this
   balloons the WCET into the billions, and inflates every integer benchmark
   10–30×. That trades a tight-but-flagged number for a sound-but-meaningless one.

Tightening past (3) needs a precise MSP430X cycle model **and** exact per-loop
trip counts; it still cannot get past the memory-indirect call in (2). The effort
is large and fragile, for a benchmark-suite artifact.

## The route to genuinely sound numbers (when needed)

Not this cost model — a **self-contained rewrite** of the benchmark, the
`tests/srcMaelardalen/sqrt.c` / `statemate` precedent: replace the libm/soft-float
calls with analyzable base-ISA C compiled into the same module, so LLTA analyzes
the *real executed code*. It is sound and tight, at the cost of per-benchmark work
and a changed benchmark. `sqrt` (self-implemented `sqrtfcn`) is the worked example.

## Foundation left in place

- `RTTarget::getExternalCallCost(name)` — the seam (default `nullopt`).
- `ProgramGraph` external-call detection + `UnsoundExternalCallees` reporting.
- `RTTarget::getInstructionLatency(const MCInst&)` — base-ISA `MCInst` latency
  (would serve an on-the-fly analyzer if MSP430X support ever lands in LLVM).
- `scripts/derive_abi_costs.py` — offline, GNU-objdump-based conservative cost
  derivation. Usable to populate the seam for routines that *are* tractable
  (self-contained counted-loop integer helpers with no indirect calls), if a
  future need justifies the precision work.
