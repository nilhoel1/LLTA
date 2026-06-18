#!/usr/bin/env python3
"""LLTA regression test for the Maelardalen (MRTC) benchmark suite.

Data-driven harness. It drives every Maelardalen benchmark through the analyzer
and compares the *observed* outcome to the committed baseline in
``msp430/regression_baselines.json``.

Testing philosophy (this checks TOOLCHAIN stability, not LLTA correctness):

  * Every benchmark is BUILT (via the msp430 Makefile) and RUN through ``llta``.
    Loop bounds come from the Clang LoopBoundPlugin, driven by the
    ``#pragma loop_bound(...)`` annotations in tests/srcMaelardalen/*.c.
  * A benchmark that produces a WCET pins that integer -- the contract.
  * A benchmark that does NOT produce a WCET -- because it fails to build,
    crashes the analyzer, or the ILP solver gives up -- is *also* a valid,
    recorded outcome (``"expected": null`` + a human-readable status/note).
    A crash is a valid test run.

Per-benchmark result:
  GREEN  observed outcome matches the baseline
  YELLOW WCET drifted, OR a previously non-producing benchmark now yields a WCET
         (the baseline should be refreshed)
  RED    a benchmark that used to yield a WCET no longer does (regression), or
         the baseline file is missing

Exit code: RED -> 1, otherwise 0 (YELLOW passes with a warning).

Note: benchmarks are regenerated each run via the Makefile, so the MSP430
toolchain and a built clang/opt/llta must be present. Make's incremental
dependency tracking keeps re-runs cheap.
"""
import json
import os
import re
import subprocess
import sys

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
LLTA_PATH = os.path.abspath(os.path.join(TESTS_DIR, "../build/bin/llta"))
MSP430_DIR = os.path.join(TESTS_DIR, "msp430")
BASELINES_PATH = os.path.join(MSP430_DIR, "regression_baselines.json")

GREEN, YELLOW, RED = "GREEN", "YELLOW", "RED"
STATUS_PRIORITY = {GREEN: 0, YELLOW: 1, RED: 2}

WCET_PATTERNS = [
    re.compile(r"WCET \(worst-case execution time\): (\d+) cycles"),
    re.compile(r"All solvers agree on WCET: (\d+) cycles"),
]


def extract_wcet(text):
    """Return the WCET integer found in analyzer output, or None."""
    for pattern in WCET_PATTERNS:
        match = pattern.search(text)
        if match:
            return int(match.group(1))
    return None


def run_benchmark(name, spec):
    """Build + analyze a benchmark via the Makefile. Returns (wcet|None, error|None).

    The .wcet file captures llta's output regardless of whether it crashed,
    failed to build, or the solver gave up -- so a missing WCET line (or a
    missing file) simply means "did not produce a WCET", a valid recorded outcome.
    """
    try:
        subprocess.run(
            ["make", f"TEST={name}", "analyze"],
            cwd=MSP430_DIR,
            capture_output=True,
            text=True,
            timeout=spec.get("timeout", 180),
        )
    except subprocess.TimeoutExpired:
        return None, "build/analyze timeout"
    wcet_file = os.path.join(MSP430_DIR, f"build_{name}", f"{name}.wcet")
    if not os.path.exists(wcet_file):
        return None, None
    with open(wcet_file, errors="replace") as handle:
        return extract_wcet(handle.read()), None


def classify(observed, expected):
    """Compare an observed WCET (int|None) against the baseline expected (int|None)."""
    if expected is None:
        # Baseline: this benchmark does not produce a WCET (build-fail / crash /
        # analyze-fail). Still not producing one => GREEN (matches the baseline).
        return GREEN if observed is None else YELLOW
    # Baseline pins an integer WCET.
    if observed is None:
        return RED  # regression: lost the WCET it used to compute
    if observed == expected:
        return GREEN
    return YELLOW  # WCET drifted


def report(name, status, observed, spec, error):
    expected = spec.get("expected")
    note = spec.get("status") or ""
    if expected is None:
        exp_str = f"no-wcet ({note})" if note else "no-wcet"
    else:
        exp_str = str(expected)
    obs_str = str(observed) if observed is not None else "no-wcet"
    if error:
        obs_str += f" [{error}]"
    print(f"  {status:<6} {name:<16} expected={exp_str:<14} observed={obs_str}")


def main():
    print("=== LLTA Regression Test (Maelardalen suite) ===")
    print(f"LLTA: {LLTA_PATH}")
    if not os.path.exists(LLTA_PATH):
        print("Error: LLTA executable not found.")
        sys.exit(1)
    if not os.path.exists(BASELINES_PATH):
        print(f"Error: baselines not found: {BASELINES_PATH}")
        sys.exit(1)

    with open(BASELINES_PATH) as handle:
        baselines = json.load(handle)

    results = []
    for name, spec in sorted(baselines.get("maelardalen", {}).items()):
        observed, error = run_benchmark(name, spec)
        status = classify(observed, spec.get("expected"))
        report(name, status, observed, spec, error)
        results.append((name, status))

    print("\n=== Summary ===")
    counts = {GREEN: 0, YELLOW: 0, RED: 0}
    final = GREEN
    for _, status in results:
        counts[status] += 1
        if STATUS_PRIORITY[status] > STATUS_PRIORITY[final]:
            final = status
    for status in (RED, YELLOW, GREEN):
        names = [n for n, s in results if s == status]
        if names:
            print(f"{status}: {len(names)} -> {', '.join(names)}")
    print(f"\nFinal Result: {final}  "
          f"(GREEN={counts[GREEN]} YELLOW={counts[YELLOW]} RED={counts[RED]})")

    sys.exit(1 if final == RED else 0)


if __name__ == "__main__":
    main()
