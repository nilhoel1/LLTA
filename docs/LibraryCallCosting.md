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

## What LLTA does today

LLTA **detects** every reachable body-less call (see `ProgramGraph::finalize` →
`chargeOrStage`). For each, it queries the target's
`RTTarget::getExternalCallCost(name)`:

- **If the target supplies a cost**, it is added to the call-site node's bound and
  the callee is *not* flagged — the WCET stays sound for that call.
- **If not** (`nullopt`), the callee is staged into `UnsoundExternalCallees`
  (surfaced by `PathAnalysisPass`) and the WCET is **reported UNSOUND**. The
  numeric WCET is still emitted but explicitly flagged as an under-approximation.

`MSP430FR5994Target::getExternalCallCost` implements the seam for the **32
`__mspabi_*` helpers** in `ABI_FINDINGS.md`'s `ok` and `STATIC-UNSOUND` regimes —
16/32-bit int mul/div/mod, all the variable shifts (incl. the 32/64-bit
`slll`/`sral`/`srll`/…), 64-bit `remlli`, and the small int↔float/double conversions
— using **measured** worst-case bounds. It uses only the `compute` column (NWAITS=0,
RAM/≤8 MHz), which is placement-independent; measurements are tight (not the loose
static bounds described below) and are sound or trustworthy (`ok` = cross-checked
`measured ≤ sound-static-ceiling`; `STATIC-UNSOUND` shifts = trustworthy measurement,
the static checker merely fails to see their loop). Costing applies only at
`-fram-wait-states=0`; at any wait state ≥ 1 the call returns `nullopt` and stays
UNSOUND, because the per-call FRAM instruction-fetch penalty is **layout-dependent**
and has no portable upper bound (the `fram_tot` column is not used — see
`ABI_FINDINGS.md`, "fram_tot is layout-dependent").

Everything else stays UNSOUND-marked: the `no-static` helpers — 64-bit mul/div
(`mpyll`/`divlli`/`divull`/`remull`), **all** soft-`float`/`double` arithmetic
(`__mspabi_addf/addd/mpyd/divd/cmpd/…`), and the 64-bit int↔float conversions
(`fltllif`/`fixdlli`/…) — plus libm (`sin`/`cos`/…) and `memcpy`. The `no-static`
helpers have **no derivable sound ceiling** (their soft-float tree reaches a
statically-unresolvable indirect call, per obstacle (2) below), so the only available
bound is the empirical feature-scan value; we do not trust it enough to drop the
UNSOUND flag. This is deliberate: a flagged under-approximation beats an unflagged
wrong number, and beats a sound-but-useless one (below).

## Why we do not compute these costs *statically* (the wall)

The integer helpers above are costed from **hardware measurement**
(`ABI_FINDINGS.md`), which sidesteps obstacles (1) and (3) below for the routines
it covers. The remainder of this section is why a *static, source-/binary-derived*
cost is impractical — and why the `no-static` soft-`double`/`float` and 64-bit
routines stay UNSOUND even though `ABI_FINDINGS.md` now reports a measured
feature-scan value for them (obstacle (2): their tree reaches a
statically-unresolvable indirect call, so **no sound static ceiling is derivable**
to cross-check the measurement against — the bound is empirical only, and we keep
the UNSOUND flag rather than trust an unverified worst case):

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

- `RTTarget::getExternalCallCost(name)` — the seam (default `nullopt`). Implemented
  by `MSP430FR5994Target` for the 32 `ok`/`STATIC-UNSOUND` `__mspabi_*` helpers from
  measured `compute` bounds (`ABI_FINDINGS.md`); still `nullopt` for the `no-static`
  FP/64-bit helpers, libm, `memcpy`, and any `-fram-wait-states >= 1` run.
- `ProgramGraph` external-call detection + `UnsoundExternalCallees` reporting.
- `RTTarget::getInstructionLatency(const MCInst&)` — base-ISA `MCInst` latency
  (would serve an on-the-fly analyzer if MSP430X support ever lands in LLVM).
- `scripts/derive_abi_costs.py` — offline, GNU-objdump-based conservative cost
  derivation. Usable to populate the seam for routines that *are* tractable
  (self-contained counted-loop integer helpers with no indirect calls), if a
  future need justifies the precision work.
