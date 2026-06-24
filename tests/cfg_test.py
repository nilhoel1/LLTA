#!/usr/bin/env python3
"""LLTA CFG/ILP robustness suite.

Small, hand-written C modules in ``tests/cfg/`` that each force one control-flow
shape (multiple calls per block, multiple returns, switch dispatch, nested/
sibling loops, noreturn call, self-recursion, indirect call, trivial). Each is
built + analyzed through the SAME MSP430 toolchain as the Maelardalen suite
(reusing ``msp430/Makefile`` with ``MODULES=../cfg``) and its observed outcome is
compared against the committed baseline in ``cfg_baselines.json``.

Philosophy mirrors regression_test.py: a benchmark that produces a WCET pins that
integer; a benchmark that does NOT (because the shape is a documented gap -- e.g.
self-recursion or an indirect call the analyzer cannot model) records
``"expected": null`` plus a human-readable status/note, and *that* is the
contract. The suite stays GREEN while behavior is unchanged and turns YELLOW the
day a documented gap starts producing a WCET (refresh the baseline) or RED if a
shape that used to analyze stops.
"""
import json
import os
import re
import subprocess
import sys

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
LLTA_PATH = os.path.abspath(os.path.join(TESTS_DIR, "../build/bin/llta"))
MSP430_DIR = os.path.join(TESTS_DIR, "msp430")
CFG_DIR = os.path.join(TESTS_DIR, "cfg")
BASELINES_PATH = os.path.join(TESTS_DIR, "cfg_baselines.json")

GREEN, YELLOW, RED = "GREEN", "YELLOW", "RED"
STATUS_PRIORITY = {GREEN: 0, YELLOW: 1, RED: 2}

WCET_PATTERNS = [
    re.compile(r"WCET \(worst-case execution time\): (\d+) cycles"),
    re.compile(r"All solvers agree on WCET: (\d+) cycles"),
]


def extract_wcet(text):
    for pattern in WCET_PATTERNS:
        match = pattern.search(text)
        if match:
            return int(match.group(1))
    return None


def run_benchmark(name, spec):
    """Build + analyze tests/cfg/<name>.c via the shared Makefile. Returns
    (wcet|None, error|None). A missing WCET line is a valid recorded outcome."""
    try:
        subprocess.run(
            ["make", f"TEST={name}", "MODULES=../cfg", "analyze"],
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
    if expected is None:
        return GREEN if observed is None else YELLOW
    if observed is None:
        return RED
    if observed == expected:
        return GREEN
    return YELLOW


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
    print(f"  {status:<6} {name:<18} expected={exp_str:<18} observed={obs_str}")


def main():
    print("=== LLTA CFG/ILP Robustness Suite ===")
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
    for name, spec in sorted(baselines.get("cfg", {}).items()):
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
