# LLTA Tests

Test cases and benchmarks for LLTA. Sources for the Maelardalen (MRTC) suite
live in `tests/srcMaelardalen/` (annotated with `#pragma loop_bound(lower, upper)`).

## MSP430 build pipeline

`tests/msp430/` has a modular Makefile. Prerequisite: `make -C tests/msp430 download`.

```bash
make -C tests/msp430 TEST=cnt all        # C -> .ll -> .opt.ll -> .S -> .elf -> .dump
make -C tests/msp430 TEST=cnt analyze    # + loop bounds (LoopBoundPlugin) + WCET, into build_cnt/cnt.wcet
```

Loop bounds: SCEV first, falling back to the plugin's JSON (from the source
pragmas). IR is optimized with `mem2reg,...,loop-rotate,indvars` so SCEV sees
canonical loops; `generate_loop_bounds.sh <arch> <target>` regenerates a JSON
standalone.

Build the whole suite at once (never aborts on one failure; prints a
`BUILD`/`BOUNDS`/`ANALYZE` table):

```bash
bash tests/msp430/build_all.sh            # .ll + .dump for every benchmark
bash tests/msp430/build_all.sh analyze    # + WCET analysis
```

Per-benchmark artifacts go to `tests/msp430/build_<name>/` and are git-ignored
(regenerated on demand); only the baselines are committed.

## Regression testing

`tests/regression_test.py` is driven by `tests/msp430/regression_baselines.json`.
It builds and runs **every** Maelardalen benchmark and compares each outcome to
its baseline:

- `"expected": <int>` — must reproduce this exact WCET.
- `"expected": null` — a valid run that yields **no** WCET (build fails, analyzer
  crashes, or the ILP gives up). A crash is a valid run; `status`/`note` say why.

```bash
./config.sh build
python3 tests/regression_test.py     # GREEN = all outcomes match
```

**GREEN** matches; **YELLOW** WCET drifted or a non-producing benchmark now
yields one (refresh the baseline); **RED** a benchmark lost its WCET (regression,
exit 1). To re-baseline, run `build_all.sh analyze` and update the affected
entries in `regression_baselines.json`.
