# MSP430FR5994 FRAM Cache — Findings

Empirical characterization of the (undocumented) MSP430FR5994 FRAM instruction
cache, measured on real silicon with the micro-benchmark in `main/cache_bench.c`.
Measured 2026-06-23 on an MSP-EXP430FR5994 Launchpad.

## Summary

| Property | Finding |
|---|---|
| Cache type | instruction-fetch read cache (does not cache data reads) |
| Associativity | **2-way** |
| Geometry | consistent with 2 sets × 2 ways × 8-byte lines (32 B) |
| **Replacement policy** | **LRU** |
| Determinism | exact across repeats → not random / tree-PLRU\* |
| Miss penalty @16 MHz | **~15 cycles** (full 8-byte line fill) |

\* For a 2-way set, true-LRU and tree-PLRU are mathematically identical, so the
distinction is moot at this associativity — "LRU" is unambiguous.

**Consequence:** the LLTA WCET analyser can use `--fram-cache-policy=lru` for the
FR5994 instead of the pessimistic `unknown` model.

---

## 1. How the cache is made observable

The FRAM cache is **invisible at ≤8 MHz**: at that speed FRAM has zero wait states,
so a hit and a miss cost the same number of CPU cycles. The cache only matters above
8 MHz, where FRAM needs wait states and the cache hides them on hits.

The benchmark times every access pattern **twice** — once at 8 MHz (NWAITS=0) and
once at 16 MHz (NWAITS=1) — and subtracts:

```
extra_cycles = cycles_16MHz − cycles_8MHz
```

The same machine code executes the same number of CPU cycles at both clock speeds
**except** for wait-state stalls on cache misses. So `extra_cycles` is exactly the
total miss penalty incurred by that pattern. Cycles are counted by **Timer_B0**,
clocked from SMCLK = MCLK (1 count per CPU cycle, stalls included).

To raise the clock safely the FRAM wait state is programmed **before** the DCO is
sped up (`FRCTL0 = FRCTLPW | NWAITS_1`, then `CSCTL1 = DCORSEL | DCOFSEL_4`); a wrong
order violates the FRAM access time and resets the chip. After measuring, the code
drops back to 8 MHz to print over UART at the known-good 9600 baud.

## 2. The probe: controlling which cache lines are touched

Each "cache line" under test is a tiny **stub function** — just a `ret` (2 bytes) —
forced into a dedicated `.probe` section by `bench.ld`. The stubs are 16-byte
aligned and 8-byte spaced, so with the FR5994 set index = **address bit 3**:

```
p0 @ ...00  set 0      p2 @ ...10  set 0      p4 @ ...20  set 0   (even → set 0)
p1 @ ...08  set 1      p3 @ ...18  set 1      ...                 (odd  → set 1)
```

`A = p0`, `B = p2`, `C = p4` are therefore three **distinct cache lines that all map
to the same 2-way set (set 0)**. Calling a stub performs exactly one instruction
fetch to its line, so a sequence of stub calls is a controlled sequence of cache
accesses into one set.

**Keeping the signal clean.** The timing loop itself is **RAM-resident**
(`.ramtext`, copied from FRAM at startup). Code executing from RAM bypasses the FRAM
cache entirely, so the loop counter, the calls, and the `TB0R` reads do **not**
pollute the cache. During the timed window the *only* FRAM instruction fetches are
the probe-stub `ret`s — i.e. exactly the intended A/B/C access pattern.

## 3. The benchmarks

All four run their pattern `K = 300` times and report `extra_cycles`. A warm-up call
first drives the cache to steady state, so cold-start misses do not skew the result.

| Experiment | Per-iteration accesses | Distinct lines (set 0) | Purpose |
|---|---|---|---|
| `hit1`    | `A`           | 1 | working set fits trivially → expect 0 misses |
| `fit2`    | `A B`         | 2 | fits a **2-way** set → expect 0 misses |
| `thrash3` | `A B C`       | 3 | exceeds 2 ways → **calibration**: 3 misses/iter under *any* policy |
| `reuse`   | `A B A C`     | 3 | **the discriminator**: LRU and FIFO differ here |

`hit1`/`fit2` establish the associativity: if 2 lines fit with no misses but 3 lines
thrash, the set holds exactly 2 ways. `thrash3` cycles 3 lines through a 2-way set,
which misses on *every* access regardless of replacement policy (3 misses/iter) — it
is the calibration reference. `reuse` is where the policy shows.

## 4. The discriminator: why `reuse` separates LRU from FIFO

The pattern is `A B A C` repeated. Tracking the 2-way set 0 to steady state
(state shown as a list, eviction victim in **bold**):

### LRU — list ordered most-recently-used → least, evict the least-recently-used
| access | resident before | hit/miss | resident after |
|---|---|---|---|
| `A` | `[C, A]` | **hit** | `[A, C]` |
| `B` | `[A, C]` | miss (evict **C**) | `[B, A]` |
| `A` | `[B, A]` | **hit** | `[A, B]` |
| `C` | `[A, B]` | miss (evict **B**) | `[C, A]` |

End state = start state. **2 misses per iteration** (B and C). The re-use of `A`
keeps it most-recently-used, so it is never evicted.

### FIFO — queue ordered oldest → newest, evict the oldest, no reorder on a hit
| access | resident before | hit/miss | resident after |
|---|---|---|---|
| `A` | `[A, C]` | hit (no reorder) | `[A, C]` |
| `B` | `[A, C]` | miss (evict **A**) | `[C, B]` |
| `A` | `[C, B]` | **miss** (A was evicted) | `[B, A]` |
| `C` | `[B, A]` | miss (evict **B**) | `[A, C]` |

End state = start state. **3 misses per iteration**. FIFO ignores the re-use of `A`,
so `A` is evicted by insertion order and misses on its second access.

So the model makes a sharp, falsifiable prediction:

```
reuse misses/iter:   LRU = 2     FIFO = 3
```

(`tools/cache_sim.py` reproduces these exactly.)

## 5. Measured data and the verdict

Captured UART (`K = 300`, `extra_cycles = cyc16 − cyc8`):

```
EXP=hit1     extra=0
EXP=fit2     extra=0
EXP=thrash3  extra=13500     → 45.0 cyc/iter
EXP=reuse#0  extra=9000      → 30.0 cyc/iter   (identical for #1,#2,#3)
```

Reasoning:

1. **2-way confirmed.** `hit1` (1 line) and `fit2` (2 lines) incur **0** extra
   cycles → those working sets fit. `thrash3` (3 lines) does not → the set holds
   exactly 2 lines.

2. **Calibrate the per-miss penalty.** `thrash3` forces **3 misses/iteration**
   (3 lines cycling through a 2-way set always miss, under any policy). It cost
   `45.0` cycles/iter, so:

   ```
   penalty = 45.0 / 3 = 15 cycles per miss
   ```

   (A miss is a full 8-byte line fill — 4 FRAM words plus refill overhead — not a
   single wait-state cycle. The exact value doesn't matter; it cancels in the ratio
   below.)

3. **Read out the `reuse` miss count.**

   ```
   reuse misses/iter = 30.0 cyc/iter ÷ 15 cyc/miss = 2.00
   ```

   Equivalently, without any penalty number, purely as a ratio against the
   calibration experiment:

   ```
   reuse / thrash3 = 9000 / 13500 = 2/3
   ⇒ reuse misses = (2/3) × 3 = 2 misses/iter
   ```

4. **Verdict.** `reuse = 2 misses/iter` matches **LRU** (prediction 2); **FIFO**
   predicts 3 (which would equal `thrash3` = 13500, not the observed 9000). The four
   `reuse` repeats are bit-identical (spread 0), so the policy is deterministic — not
   random or pseudo-random.

   **The MSP430FR5994 FRAM cache uses LRU replacement.**

## 6. Reproduce

```sh
make bench                                  # build the gcc image (no LLTA, no FreeRTOS)
make flash-bench                            # flash it (add sudo only if not in dialout)
python3 uart.py | tee bench.log             # firmware re-prints ~1 s apart; Ctrl-C after a block
python3 tools/measure_cache.py --uart-log bench.log
```

`measure_cache.py` performs the calibration in §5 automatically and prints the
policy verdict. `tools/cache_sim.py` (run standalone) prints the LRU/FIFO predictions
of §4.

## 7. Side notes (LLTA)

The original goal was to cross-check against the LLTA WCET analyser on the
byte-identical merged ELF. Two LLTA issues surfaced (left unfixed):

- `--fram-cache-policy=fifo` **hangs** (never reaches path analysis; `unknown`/`lru`
  complete in seconds) — a likely non-converging FIFO fixpoint.
- The clang+LLTA **merged image crashes at runtime** on hardware (silent UART). The
  gcc build runs correctly and is the measurement vehicle here. LLTA also only
  cache-models the analysed function's own FRAM fetches, not called stubs, so it
  reports ~0 penalty on this stub-call probe and is not the policy oracle.

The hardware measurement above is self-contained and needs neither LLTA nor FreeRTOS.
