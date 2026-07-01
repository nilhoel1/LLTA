# MSP430 EABI Runtime-Call Cycle Bounds — Findings

Measured worst-case cycle bounds for the `__mspabi_*` runtime helpers (libgcc) that the
LLTA WCET analyser cannot cost — they are body-less in the IR, MSP430X-encoded, and the
soft-float ones additionally reach a statically-unresolvable indirect-call tree — so LLTA
charges them **0 cycles** and any WCET over code that calls them is reported UNSOUND.
Measured on an MSP-EXP430FR5994 Launchpad with `src/abi_bench.c` (same rig as the
FRAM-cache work in `FINDINGS.md`).

- **Integer 16/32-bit** set (13 helpers): 2026-06-23.
- **Float / double / conversions / 64-bit / wider-shift** set (41 helpers): 2026-07-01.

## Results

`compute` = worst-case cycles/call at **8 MHz, NWAITS=0** (no instruction-fetch wait
states; cache- and placement-independent — the bound if the helper runs from RAM or at
≤8 MHz). `fram_tot` = a single **cold-cache** call at **16 MHz, NWAITS=1** (cache flushed
first): compute plus the worst-case cold instruction-fetch penalty. `status` = the
`derive_abi_costs.py` sound-ceiling cross-check (see *Soundness*).

### Integer, 16/32-bit (original set)

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_mpyi`  | 304  | 1879  | b = 0x8000 (max MSB position) | ok |
| `__mspabi_divi`  | 451  | 2086  | 0x7FFF / 1 | ok |
| `__mspabi_divu`  | 443  | 2003  | 0xFFFF / 1 | ok |
| `__mspabi_remi`  | 468  | 2103  | 0x7FFF % 1 | ok |
| `__mspabi_remu`  | 444  | 2004  | 0xFFFF % 1 | ok |
| `__mspabi_slli`  | 89   | 134   | count = 15 | **STATIC-UNSOUND** |
| `__mspabi_srai`  | 89   | 134   | count = 15 | **STATIC-UNSOUND** |
| `__mspabi_srli`  | 104  | 149   | count = 15 | **STATIC-UNSOUND** |
| `__mspabi_mpyl`  | 981  | 4431  | 0xFFFFFFFF × 0xFFFFFFFF | ok |
| `__mspabi_divli` | 1624 | 9934  | 0x7FFFFFFF / 1 | ok |
| `__mspabi_divlu` | 1601 | 9866  | 0xFFFFFFFF / 1 | ok |
| `__mspabi_remli` | 1840 | 10225 | 0x7FFFFFFF % 1 | ok |
| `__mspabi_remul` | 1603 | 9868  | 0xFFFFFFFF % 1 | ok |

### Integer, 64-bit

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_mpyll`  | 4120  | 18625 | 0xF…F × 0xF…F | no-static |
| `__mspabi_divlli` | 12041 | 44096 | 0x7F…F / 1 | no-static |
| `__mspabi_divull` | 12029 | 44384 | 0xF…F / 1 | no-static |
| `__mspabi_remlli` | 9614  | 39164 | 0x7F…F % 1 | ok |
| `__mspabi_remull` | 9883  | 39688 | 0xF…F % 1 | no-static |

### Variable shifts, 32/64-bit

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_slll`  | 215 | 260 | count = 31 | **STATIC-UNSOUND** |
| `__mspabi_sral`  | 215 | 245 | count = 31 | **STATIC-UNSOUND** |
| `__mspabi_srll`  | 246 | 291 | count = 31 | **STATIC-UNSOUND** |
| `__mspabi_sllll` | 508 | 583 | count = 63 | ok |
| `__mspabi_srall` | 508 | 583 | count = 63 | ok |
| `__mspabi_srlll` | 563 | 638 | count = 63 | ok |

### Single-precision float

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_addf` | 1623 | 3678  | denormal + denormal | no-static |
| `__mspabi_subf` | 1638 | 6948  | near-equal (1-ulp cancellation) | no-static |
| `__mspabi_mpyf` | 3331 | 12571 | max × denormal | no-static |
| `__mspabi_divf` | 1789 | 6394  | denormal / 1 | no-static |
| `__mspabi_cmpf` | 911  | 3446  | (near-constant) | no-static |

### Double-precision float

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_addd` | 7302  | 27597  | denormal + denormal | no-static |
| `__mspabi_subd` | 7739  | 31754  | near-equal (1-ulp cancellation) | no-static |
| `__mspabi_mpyd` | 25898 | 101880 | max × denormal | no-static |
| `__mspabi_divd` | 11907 | 51927  | max / denormal | no-static |
| `__mspabi_cmpd` | 1241  | 4706   | (near-constant) | no-static |

### int → float / double

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_fltif`   | 527   | 1637  | 0xFFFF (16-bit) | ok |
| `__mspabi_fltuf`   | 500   | 1520  | (near-constant) | ok |
| `__mspabi_fltlif`  | 515   | 1565  | 0xFFFFFFFF (32-bit) | ok |
| `__mspabi_fltulf`  | 491   | 1466  | (near-constant) | ok |
| `__mspabi_fltllif` | 23986 | 87534 | 0x7F…F (64-bit) | no-static |
| `__mspabi_fltullf` | 24154 | 88017 | 0xF…F (64-bit) | no-static |
| `__mspabi_fltid`   | 867   | 2187  | 0xFFFF | ok |
| `__mspabi_fltud`   | 838   | 2068  | (near-constant) | ok |
| `__mspabi_fltlid`  | 855   | 2115  | 0xFFFFFFFF | ok |
| `__mspabi_fltuld`  | 829   | 2014  | (near-constant) | ok |
| `__mspabi_fltllid` | 23239 | 85407 | 0x7F…F (64-bit) | no-static |
| `__mspabi_fltulld` | 23443 | 86061 | 0xF…F (64-bit) | no-static |

### float / double → int, and float ↔ double

| helper | compute | fram_tot | worst-case input | status |
|---|--:|--:|---|---|
| `__mspabi_fixfli`  | 485   | 1025   | (near-constant) | ok |
| `__mspabi_fixflli` | 44133 | 169744 | max float (→int64 overflow/saturate) | no-static |
| `__mspabi_fixdli`  | 2683  | 11833  | (near-constant) | ok |
| `__mspabi_fixdlli` | 48931 | 188177 | max double (→int64 overflow/saturate) | no-static |
| `__mspabi_fixdul`  | 5668  | 24898  | (near-constant) | no-static |
| `__mspabi_fixdull` | 48496 | 186496 | max double (→uint64 overflow/saturate) | no-static |
| `__mspabi_cvtfd`   | 1047  | 2427   | (near-constant) | ok |
| `__mspabi_cvtdf`   | 3179  | 13004  | (near-constant) | ok |

**The float/double→int64 and int64→float/double conversions are the sharpest surprise:**
`fixdlli`/`fixdull` cost ~49k cycles compute and ~188k cold, and the int64↔float family
~24k — all silently costed **0** by LLTA today. The max-magnitude / out-of-range inputs
(overflow-to-saturation) are the worst corner for the `fix*` routines.

## Method — feature-class input scan (not enumeration)

Enumerating operands is infeasible and cycle count is **not monotonic in the operand
value**, but each routine's cost is a function of a low-cardinality **feature**, so we scan
one representative per feature class and take the max (`src/abi_bench.c`):

| routine class | cost feature | scanned reps |
|---|---|---|
| integer multiply | MSB position (+ popcount) | all-ones prefixes, adversarial multiplicands |
| integer divide/mod | bitlen(dividend) − bitlen(divisor) | 2^k−1 dividend × divisor {1,2,3} |
| variable shift | shift count | 0…width (16/32/64) |
| **float/double add/sub** | exponent-difference alignment + cancellation normalization | exp diffs 0…mantissa-width, 1-ulp cancellation pair, ±0/denormal/Inf/NaN/max |
| **float/double mul/div** | mantissa loop (≈fixed) + denormal pre/post-normalization | normal×normal, denormal operands, min/max magnitude, specials |
| **float/double compare** | (near-constant) | less/equal/greater, NaN, ±0, Inf |
| **int→float/double** | leading-zero count of the integer | 0, 1, 2^k−1, MAX across width |
| **float/double→int** | exponent (shift count) + overflow/saturation | 0, 1, 2^31, 2^62, 2^63, max, Inf, NaN |

The recorded worst inputs confirm the scan reached each routine's cost-driving corner
(add/sub → cancellation & denormals; mul → denormal operand; `fix*` → overflow-saturating
max). Each measurement is a **single call** (the MSP430 is deterministic; a single call is
exact, no averaging). The 13 original integer helpers use the 16-bit Timer_B path; the
larger new helpers use the 32-bit path (`time_fn32`) because a cold call can exceed the
16-bit range — e.g. `mpyd` cold = 101880 and `fixdlli` cold = 188177 would have **silently
wrapped** the old counter. `ovf=0` on every row confirms no wrap occurred.

## Soundness — three regimes

`derive_abi_costs.py` (in LLTA) computes a provably-sound but loose static ceiling per
routine from the linked ELF; `tools/abi_costs.py` runs it and requires **measured ≤
static**. The 54 helpers split into three regimes:

1. **`ok` (26 helpers)** — a sound ceiling exists and measured ≤ it (16/32-bit int
   mul/div/mod, 64-bit shifts, `remlli`, and the small int↔float conversions). The
   measurement is far tighter than the (often astronomically loose, e.g. 4×10¹⁴ for
   `fltuf`) ceiling, so it is both **tight and safe**.

2. **`STATIC-UNSOUND` (6 helpers)** — the variable shifts `slli/srai/srli` **and** their
   32-bit siblings `slll/sral/srll`. `derive_abi_costs.py`'s address-interval loop
   detection misses the shift loop (its back-edge targets code *before* the symbol entry,
   so no back-edge is seen and the routine is costed as straight-line ~24–56 cyc). The
   **measured value is the trustworthy bound** for these.

3. **`no-static` (22 helpers)** — 64-bit mul/div, **all** float/double arithmetic, and the
   64-bit int↔float conversions. `derive_abi_costs.py` returns no ceiling because the
   soft-float routines reach a **statically-unresolvable indirect call** (`addf` unpacks
   its operands and `call`s a shared pack/normalize core through a register — exactly the
   limitation LLTA's own `RTTarget.h` documents). **No sound ceiling is derivable for
   these**, so the measured feature-scan value is the *only* available bound, and its
   soundness is **empirical** (the scan hit the cost-driving corner, as the worst inputs
   show) — not proven. This is the honest status: for the FP/64-bit helpers the bound
   rests on the completeness of the feature scan, and these routines are bounded-loop, so
   the scanned corners bracket the maximum.

## `fram_tot` is layout-dependent; `compute` is not

Re-measuring the 13 original helpers alongside the 41 new ones reproduced every `compute`
value **bit-identically** (304, 451, 443, 89, 981, 1624, …) — confirming `compute` is a
stable, placement-independent property of the routine and that the measurement method is
unchanged. But **5 of the 13 `fram_tot` values shifted** purely because the binary grew and
relocated the helpers in FRAM: `mpyi` 919→1879, `mpyl` 4911→**4431** (it *decreased*),
`srai` 119→134, `srli` 134→149, `remli` 10240→10225. `fram_tot` is the cold
instruction-fetch penalty, which depends on where a routine's loop body lands relative to
the 2-set / 2-way / 8-byte FRAM cache (conflict misses per iteration), i.e. on link-time
placement.

**Consequence:** use `compute` as the portable per-call cost. `fram_tot` is a per-build
indication of the cold-cache penalty, **not** a universal upper bound — a different link
order can exceed the value measured here. For a sound FRAM@16 MHz bound, combine `compute`
with the worst-case cache penalty characterised in `FINDINGS.md` (miss = 15 cyc, 2×2×8 B
LRU) rather than treating `fram_tot` as a constant.

## Caveat — hardware-multiplier routing

The soft `__mspabi_mpyi`/`mpyl`/`mpyll` measured here are emitted only when the compiler is
invoked **without** the hardware multiplier. Under `-mhwmult` (the `f5series` variant is the
default for the FR5994), integer multiplies route to `__mspabi_mpyi_f5hw` /
`mpyl_f5hw` / `mpyll_f5hw`, which drive the MPY32 peripheral and are far cheaper — so on a
hardware-multiplier build the soft-multiply rows above do **not** describe what runs. The
HW variants are out of scope for this round. **Verification step:** disassemble a merged
Mälardalen ELF (`msp430-elf-objdump -d build/mael/mael.elf`) and list which `__mspabi_*`
symbols it actually calls, to know which rows apply. There is no hardware floating point on
the FR5994, so the float/double helpers always route to the soft `*f`/`*d` routines measured
here.

## Reproduce

```sh
make abi && make flash-abi                                   # build + flash the gcc EABI bench
python3 ../uart.py | tee results/abi.log                     # capture one ABI_BENCH begin..end block
python3 tools/abi_costs.py --uart-log results/abi.log --elf build/bench.elf
```

`tools/abi_costs.py` prints the table above (with the `derive_abi_costs.py` ceiling and
status) plus a paste-ready `{"name", cycles}` table for LLTA's
`MSP430Target::getExternalCallCost()` — one column for the FRAM@16 MHz deployment
(`fram_tot`) and one for compute-only/RAM (`compute`). Wiring the table into LLTA was
intentionally left out of scope. Given the `fram_tot` layout-sensitivity above, prefer the
`compute` column plus a separately-derived cache penalty for a portable, sound bound.
