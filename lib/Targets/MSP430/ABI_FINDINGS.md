# MSP430 EABI Runtime-Call Cycle Bounds — Findings

Measured worst-case cycle bounds for the integer `__mspabi_*` soft-arithmetic helpers
(libgcc) that the LLTA WCET analyser cannot cost (body-less in the IR, MSP430X-encoded)
— so they are otherwise charged 0 cycles and the WCET is reported UNSOUND. Measured on
an MSP-EXP430FR5994 Launchpad, 2026-06-23, with `main/abi_bench.c` (same rig as the
FRAM-cache work in `FINDINGS.md`).

## Results

| helper | compute (cyc) | fram_tot (cyc) | worst-case input |
|---|--:|--:|---|
| `__mspabi_mpyi`  | 304  | 919   | b = 0x8000 (max MSB position) |
| `__mspabi_divi`  | 451  | 2086  | 0x7FFF / 1 |
| `__mspabi_divu`  | 443  | 2003  | 0xFFFF / 1 |
| `__mspabi_remi`  | 468  | 2103  | 0x7FFF % 1 |
| `__mspabi_remu`  | 444  | 2004  | 0xFFFF % 1 |
| `__mspabi_slli`  | 89   | 134   | count = 15 |
| `__mspabi_srai`  | 89   | 119   | count = 15 |
| `__mspabi_srli`  | 104  | 134   | count = 15 |
| `__mspabi_mpyl`  | 981  | 4911  | 0xFFFFFFFF × … |
| `__mspabi_divli` | 1624 | 9934  | 0x7FFFFFFF / 1 |
| `__mspabi_divlu` | 1601 | 9866  | 0xFFFFFFFF / 1 |
| `__mspabi_remli` | 1840 | 10240 | 0x7FFFFFFF % 1 |
| `__mspabi_remul` | 1603 | 9868  | 0xFFFFFFFF % 1 |

- **compute** — worst-case cycles/call at **8 MHz, NWAITS=0** (no instruction-fetch
  wait states). Cache- and placement-independent; this is the bound if the helper
  runs from RAM or at ≤8 MHz.
- **fram_tot** — a single **cold-cache** call at **16 MHz, NWAITS=1** (cache flushed
  first): compute **plus the worst-case instruction-fetch penalty**. This is the bound
  for the default FRAM-resident deployment at 16 MHz.

The gap is large for the big routines (e.g. `divli` 1624 → 9934) because their loop
body far exceeds the 32-byte (4-line) FRAM cache, so every loop iteration re-misses —
~8300 extra cycles ≈ 550 line-fills × 15 cyc (the per-line miss penalty measured in
`FINDINGS.md`). Shifts barely change (small code that fits the cache).

## Method

The rig is reused verbatim from the cache benchmark (`main/bench_common.{h,c}`):
Timer_B0 cycle counter, the 8/16 MHz clock switch with the FRAM wait state, a
RAM-resident timing path (so the harness's own fetches don't perturb the FRAM cache),
and the cache-flush routine.

Each helper is invoked directly via an `extern` prototype (args/return in R12–R15).
A measurement is a **single call** timed with Timer_B0, minus a single no-call
baseline (so the difference is the call's cost). Single calls (≤ ~1800 cyc) never wrap
the 16-bit timer; the MSP430 is deterministic, so a single call is exact (no
averaging). Earlier loop-averaged versions silently **wrapped** on the slowest divide
inputs and under-reported them — the `ovf` flag caught this and the single-call method
fixed it.

### Worst-case inputs — feature-class scan, not enumeration

Enumerating operands is infeasible (32-bit divide = 2⁶⁴) and cycle count is **not
monotonic in the operand value**. But each routine's cost is a function of a
low-cardinality **feature**, so we scan one representative per feature class and take
the max:

| routine | cost feature | scanned reps | worst |
|---|---|---|---|
| multiply | MSB position (+ popcount) | all-ones prefixes 0x1…0xFFFF | top bit set |
| divide / mod | bitlen(dividend) − bitlen(divisor) | dividend 2^k−1 × divisor {1,2,3} | max / 1 |
| variable shift | shift count | 0…15 (resp. 0…31) | max count |

The measured worst inputs (`0x7FFFFFFF / 1` for divide, count 15 for shift) match the
algorithm-predicted corners, confirming the scan found the true maximum.

## Safety: cross-check against the sound static ceiling

`LLTA/scripts/derive_abi_costs.py` computes a **provably sound** (but very loose:
LOOP_BOUND=64, 8 cyc/instr) upper bound per routine. `tools/abi_costs.py` runs it and
requires **measured ≤ static**:

- mul / div / rem: measured is **10×–10000× tighter** than the sound ceiling and well
  under it (e.g. `divli` 9934 ≪ 40 280 377) → tight **and** safe.
- **Static is UNSOUND for the variable shifts** (`slli`/`srai`/`srli`): it reports
  ~48 cyc because its address-interval loop detection **misses the loop** — the shift
  loop's back-edge (`jnz`) targets code *before* the symbol entry, so no back-edge is
  seen and the routine is costed as straight-line. The real routine loops `count`
  times, which the hardware measurement captures. **For these, the measured value is
  the trustworthy bound, not the static one.**

## Reproduce

```sh
make abi && make flash-abi                       # build + flash the gcc EABI bench
python3 uart.py | tee abi.log                    # capture (firmware re-prints in a loop)
python3 tools/abi_costs.py --uart-log abi.log --elf build/msp430-freertos.elf
```

`tools/abi_costs.py` prints the table above plus a paste-ready
`{"name", cycles}` table for LLTA's `MSP430Target::getExternalCallCost()` — one set for
the FRAM @16 MHz deployment (`fram_tot`) and one for compute-only/RAM (`compute`). Pick
the column matching the LLTA `--fram-wait-states` configuration. The measured value is
the call-site cost (call+marshal+body+ret) at the worst-case input — a safe, slightly
conservative upper bound for the callee body LLTA charges per call. Wiring the table
into LLTA was intentionally left out of scope.
