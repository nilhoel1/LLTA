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
