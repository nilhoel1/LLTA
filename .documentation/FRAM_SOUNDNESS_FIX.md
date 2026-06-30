# MSP430FR5994 FRAM timing model — soundness fix

A WCET analyser must report a **guaranteed upper bound**. The FR5994 FRAM model
previously **under-estimated** (returned values below the real execution time),
which defeats its purpose. This note records what was unsound and how it is now
sound. Soundness rule throughout: when in doubt, over-approximate.

## What was unsound

1. **Miss penalty too small.** `FRAMCacheAnalysisPass` charged a cache miss
   `MissPenalty = FRAMWaitStates` (~1 cycle) — i.e. it modelled a miss as one
   *direct-access word wait state*. The silicon services a fetch miss with a
   full **8-byte line fill (~15 cycles)**. The two are physically different
   costs that were conflated.

2. **Data operands cost nothing.** A data load/store that was not provably
   stack/SRAM emitted a cache `Barrier` (which perturbs cache state) but added
   **0 cycles**. A read/write whose address may be in FRAM (`.rodata` const
   tables, or any unproven address) incurs a real wait state that went
   uncounted.

3. **No-cache pass had the same blind spot.** `FRAMWaitStatePass` charged
   `FRAMWaitStates × fetch-words` only — no data accesses.

Net effect: `bs` reported 246 vs. 702 measured, `fibcall` 392 vs. 522 — both
under the real time, i.e. unsound.

## The fix

- **Two parameterised, physically distinct knobs.**
  - `--fram-wait-states` (default 0) — per-16-bit-word direct/data access cost.
  - `--fram-line-fill-cycles` (default **15**) — per-miss full line fill, used
    as the cache `MissPenalty`. New option; sound FR5994 default.
- **Charge unproven/FRAM data accesses.** `CacheEvent::barrier` now carries a
  `Cost`; the must (WCET) engine adds it. `FRAMAccessMapper` emits the barrier
  with `FRAMWaitStates × framDataAccessWords(MI)`. The no-cache
  `FRAMWaitStatePass` adds the same data-access term. Soundness rule:
  *unknown address ⇒ assume FRAM ⇒ charge* (`framDataAccessWords` returns 0
  only when the access is provably stack/SRAM).
- **Shared classifier.** `isProvablyStackOnly` / `framDataAccessWords` live in
  `Utility/DataMemoryAccess.{h,cpp}`, used by both passes.
- **Base latency, lower 64 KB.** The SLAU445I 0-wait CPU-cycle table is an upper
  bound for lower-64KB (16-bit) operation: the CPUX "upper 64 kb" caveat is
  about 20-bit extended addressing, which only *adds* cycles and does not occur
  here. All memory overhead is now charged additively by the two penalties.
  (Documented at the source; no code change needed.)

## Invariants preserved

- **Model off** (`--fram-wait-states=0`, no `--fram-cache`): both passes
  early-return; `--fram-line-fill-cycles` is read only with `--fram-cache`.
  Regression baselines are byte-for-byte unchanged (e.g. bs 186, fibcall 327,
  crc 100588), verified including `--fram-wait-states=0 --fram-cache` (no-ops).
- **Must-analysis structure** unchanged: charge a miss unless a hit is proven.
- **`unknown` replacement policy** stays the sound default; `lru`/`fifo` are
  opt-in tighter (and only sound if the device matches).

## Validation (`-fram-start=0x4000 -fram-wait-states=1 -fram-cache`)

| bench   | measured cyc16 | before | after  | sound? |
|---------|----------------|--------|--------|--------|
| bs      | 702            | 246    | 993    | yes    |
| fibcall | 522            | 392    | 1317   | yes    |
| crc     | 113133         | 120966 | 417633 | yes    |

`after >= measured` for all (over-estimation is acceptable; the `unknown`
policy is deliberately adversarial). Line-fill was **not** tuned to a target —
15 is the physical FR5994 value.

## Tests

- `LLTACacheModuleTests` — stub-mapper tests over `CacheAnalysis`: a cold fetch
  costs the line-fill `MissPenalty` (then a free hit); a data-access `Barrier`
  is charged its cost in must mode and 0 in may mode; zero knobs ⇒ 0.
- `LLTAMachineFunctionGraphTests` — `framDataAccessWords` classification:
  stack ⇒ 0, unknown/no-memoperand ⇒ charged, load+store ⇒ 2, non-memory ⇒ 0.
- Regression suite stays all-GREEN (model-off invariant).

---

# Tightening: target-independent data-address resolution

The soundness fix above made the model never under-estimate, but it
**over-estimated** data accesses: any access not provably *stack* was assumed
FRAM and charged a wait state, even when it targets a global in SRAM
(`.data`/`.bss`, below `FRAMStart`). This section records how that pessimism is
removed without weakening soundness.

## What is now proven SRAM

A data access keeps a link to its IR object after instruction selection
(`MachineMemOperand::getValue()`). The classifier now recovers it
target-independently — `getValue()` → `getUnderlyingObject()`
(`llvm/Analysis/ValueTracking.h`) — and drops the FRAM charge when the base is:

- an `AllocaInst` (stack/SRAM), or
- a `GlobalValue` whose **link address** (looked up by name in the ELF-derived
  `TAR.DataObjects`) is `< FRAMStart`.

Everything else — null `getValue()`, an unknown global (not in the symbol
table), an address `>= FRAMStart` (e.g. `.rodata` const tables), or a
computed/non-global base — is charged exactly as before. The resolution runs
once per function (`computeDataAccessWords`, `Utility/InstructionWords.cpp`); the
new `framDataAccessWords(MI, FRAMStart, Resolve)` overload
(`Utility/DataMemoryAccess.cpp`) does the per-operand classification.

## Why it stays sound

- We drop a charge **only on proof** of SRAM/stack residency; every unproven
  case stays conservative.
- **No new cache assumption.** Provably-stack accesses already produced 0 words
  ⇒ no `Barrier` and no cost in the cache path. Enlarging the "provably SRAM"
  set just lets more accesses take that same, already-shipped treatment. An SRAM
  access physically cannot fill or evict the FRAM instruction-fetch cache
  (separate memory, not behind the cache), so not flushing is correct.
- **Model-off / no-ELF unchanged.** With no ELF, `TAR.DataObjects` is empty, the
  resolver returns "unknown" for everything, and the counts equal the
  conservative overload's. Regression stays all-GREEN; cnt/cover unchanged.

## Alternatives rejected (target-independence constraint)

- *IR-level constant propagation* (SCCP/InstCombine/CVP) cannot supply a global's
  link address — it is a relocation, never an IR constant — and adding an IR pass
  risks perturbing the byte-for-byte model-off baselines. Not adopted.
- *Decoding the address from the ELF instruction operand*
  (`MCInstrAnalysis::evaluateMemoryOperandAddress`) is precise but needs a
  target-specific `MSP430MCInstrAnalysis` (MSP430 registers none), so it breaks
  target-independence; its coverage overlaps the IR-value path anyway. Left as a
  clean future opt-in.

## Validation (`-fram-start=0x4000 -fram-wait-states=1 -fram-cache`)

| bench   | measured cyc16 | sound (pessimistic) | after resolution | sound? |
|---------|----------------|---------------------|------------------|--------|
| bs      | 702            | 993                 | 833              | yes (tighter) |
| fibcall | 522            | 1317                | 1317             | yes (no globals) |
| crc     | 113133         | 417633              | 397770           | yes (tighter) |

`after >= measured` for all; bs and crc tightened, fibcall has no array globals
so it is unchanged (as predicted). bs dropping confirms the MSP430 backend
preserves `MMO->getValue()` for global accesses, so the mechanism is active.
