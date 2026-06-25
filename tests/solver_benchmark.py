#!/usr/bin/env python3
"""
ILP solver benchmark harness for LLTA (Gurobi vs HiGHS).

Measures, systematically and over repeated runs, the time the ILP solver spends
and the peak memory footprint of the analysis process, for each Maelardalen
(MRTC) benchmark and each ILP backend. Intended to produce CSV tables for a paper.

What is measured per run:
  - solve_time_ms : the ILP-solver time reported by llta itself. We instrumented
                    PathAnalysisPass to print a parseable line
                        ILP solve time: <ms> ms
                    timing the full backend cost (model construction + optimize).
                    This isolates the solver from the dominant LLVM codegen /
                    ProgramGraph / abstract-interpretation pipeline that runs
                    before it.
  - peak_rss_bytes: peak resident set size of the whole llta process, captured
                    externally via /usr/bin/time. This is a whole-process proxy
                    for memory footprint (isolating the solver's own allocations
                    in-process is not practical); report it as such.
  - wcet          : the WCET cycle count, used to (a) sanity-check the run and
                    (b) verify both backends agree.

Note: llta's default `-ilp-solver=auto` resolves to HiGHS, and there is no
working `all` mode in the solve path, so each backend is run in a separate
invocation with an explicit `-ilp-solver=` flag.

Usage:
    python3 tests/solver_benchmark.py                      # all benchmarks, both solvers
    python3 tests/solver_benchmark.py --benchmarks cnt,cover --reps 3
    python3 tests/solver_benchmark.py --solvers highs --skip-build
"""

import argparse
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import csv

# --- Paths -------------------------------------------------------------------
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MSP430_DIR = os.path.join(HERE, "msp430")
SRC_DIR = os.path.join(HERE, "srcMaelardalen")
LLTA = os.path.join(REPO, "build", "bin", "llta")

# Backend code-gen flags, mirroring tests/msp430/Makefile (LLCFLAGS), minus the
# -filetype=asm that only applies to the -llc transform step.
LLC_FLAGS = ["--dwarf-version=4", "--strict-dwarf", "-mtriple=msp430",
             "-mcpu=msp430x"]

# --- Parsing patterns --------------------------------------------------------
RE_WCET = re.compile(r"WCET \(worst-case execution time\): (\d+) cycles")
RE_SOLVE_MS = re.compile(r"ILP solve time: ([0-9.]+) ms")
# macOS `/usr/bin/time -l`:  "      1163264  maximum resident set size"
RE_RSS_MACOS = re.compile(r"(\d+)\s+maximum resident set size")
# GNU `/usr/bin/time -v`:    "  Maximum resident set size (kbytes): 1163264"
RE_RSS_GNU = re.compile(r"Maximum resident set size \(kbytes\): (\d+)")

IS_MACOS = platform.system() == "Darwin"


# --- Discovery ---------------------------------------------------------------
def all_benchmarks():
    """All Maelardalen benchmark names, from the source directory."""
    names = []
    for f in sorted(os.listdir(SRC_DIR)):
        if f.endswith(".c"):
            names.append(f[:-2])
    return names


def artifacts(name):
    """Return (opt_ll, elf, json) absolute paths for a benchmark."""
    bd = os.path.join(MSP430_DIR, f"build_{name}")
    return (os.path.join(bd, f"{name}.opt.ll"),
            os.path.join(bd, f"{name}.elf"),
            os.path.join(bd, f"{name}.loop_bounds.json"))


def artifacts_present(name):
    return all(os.path.isfile(p) for p in artifacts(name))


def build(name, timeout):
    """Build analysis artifacts via the Makefile. Returns True if present after."""
    try:
        subprocess.run(["make", f"TEST={name}", "analyze"],
                       cwd=MSP430_DIR, capture_output=True, text=True,
                       timeout=timeout)
    except subprocess.TimeoutExpired:
        pass
    except Exception as e:  # noqa: BLE001 - report and continue
        print(f"  build error for {name}: {e}", file=sys.stderr)
    return artifacts_present(name)


# --- One measured run --------------------------------------------------------
def run_once(name, solver, timeout):
    """
    Run llta once under /usr/bin/time. Returns a dict:
      {ok, wcet, solve_time_ms, peak_rss_bytes, error}
    `ok` is True only when a positive WCET was produced.
    solve_time_ms / peak_rss_bytes are captured whenever present, even on a
    failed solve (e.g. a backend that returns no WCET still spent time).
    """
    opt_ll, elf, js = artifacts(name)
    bd = os.path.join(MSP430_DIR, f"build_{name}")

    llta_cmd = [LLTA, f"-ilp-solver={solver}",
                f"-loop-bounds-json={js}", f"-elf-file={elf}",
                *LLC_FLAGS, opt_ll, "-o", "/dev/null"]
    time_cmd = ["/usr/bin/time", "-l"] if IS_MACOS else ["/usr/bin/time", "-v"]
    cmd = time_cmd + llta_cmd

    res = {"ok": False, "wcet": None, "solve_time_ms": None,
           "peak_rss_bytes": None, "error": None}
    try:
        p = subprocess.run(cmd, cwd=bd, capture_output=True, text=True,
                           timeout=timeout)
    except subprocess.TimeoutExpired:
        res["error"] = "timeout"
        return res
    except Exception as e:  # noqa: BLE001
        res["error"] = str(e)
        return res

    out, err = p.stdout, p.stderr

    m = RE_WCET.search(out)
    if m:
        res["wcet"] = int(m.group(1))
        res["ok"] = res["wcet"] > 0
    m = RE_SOLVE_MS.search(out)
    if m:
        res["solve_time_ms"] = float(m.group(1))

    m = RE_RSS_MACOS.search(err) if IS_MACOS else RE_RSS_GNU.search(err)
    if m:
        rss = int(m.group(1))
        res["peak_rss_bytes"] = rss if IS_MACOS else rss * 1024  # GNU=kbytes

    if not res["ok"] and res["error"] is None:
        res["error"] = "no-wcet"
    return res


# --- Stats helpers -----------------------------------------------------------
def _stats(values):
    """mean/median/std/min/max for a list of floats (empty -> all None)."""
    if not values:
        return {k: None for k in ("mean", "median", "std", "min", "max")}
    return {
        "mean": statistics.mean(values),
        "median": statistics.median(values),
        "std": statistics.pstdev(values) if len(values) > 1 else 0.0,
        "min": min(values),
        "max": max(values),
    }


def _fmt(x, nd=3):
    return "" if x is None else f"{x:.{nd}f}"


def _to_float(s):
    return float(s) if s not in ("", None) else None


def load_summary_csv(path):
    """Reconstruct (summary_rows, solvers) from a summary CSV, so the LaTeX
    table can be regenerated without re-running the sweep."""
    rows, solvers = [], []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            if r["solver"] not in solvers:
                solvers.append(r["solver"])
            rows.append({
                "benchmark": r["benchmark"], "solver": r["solver"],
                "n_ok": int(r["n_ok"]), "n_total": int(r["n_total"]),
                "wcet": r["wcet"] or None,
                "solve_ms": {"mean": _to_float(r["solve_ms_mean"]),
                             "median": _to_float(r["solve_ms_median"]),
                             "std": _to_float(r["solve_ms_std"]),
                             "min": _to_float(r["solve_ms_min"]),
                             "max": _to_float(r["solve_ms_max"])},
                "rss_bytes": {"mean": _to_float(r["rss_bytes_mean"]),
                              "median": _to_float(r["rss_bytes_median"]),
                              "std": _to_float(r["rss_bytes_std"]),
                              "min": _to_float(r["rss_bytes_min"]),
                              "max": _to_float(r["rss_bytes_max"])},
            })
    return rows, solvers


# --- LaTeX table -------------------------------------------------------------
SOLVER_LABEL = {"gurobi": "Gurobi", "highs": "HiGHS"}
MIB = 1024.0 * 1024.0


def _tex_escape(s):
    return s.replace("_", r"\_")


def latex_table(summary_rows, solvers):
    """
    A copy-pastable booktabs LaTeX table: per solver, ILP solve time
    (median +- std, ms) and peak memory (median, MiB). Only the metrics that
    matter for the paper -- time, memory, and time variability -- are kept.
    Requires \\usepackage{booktabs} in the document preamble.
    """
    # Index: (benchmark, solver) -> summary dict.
    by = {(s["benchmark"], s["solver"]): s for s in summary_rows}
    benches = sorted({s["benchmark"] for s in summary_rows})

    ncol = len(solvers)
    lines = []
    lines.append("% Requires \\usepackage{booktabs} in the preamble.")
    lines.append(r"\begin{table}[t]")
    lines.append(r"  \centering")
    lines.append(r"  \caption{ILP solver time and peak memory footprint on the "
                 r"M\"alardalen benchmarks. Time is the median over 10 runs "
                 r"$\pm$ population standard deviation; memory is the median "
                 r"peak resident set size of the analysis process.}")
    lines.append(r"  \label{tab:ilp-solver-bench}")
    lines.append(r"  \begin{tabular}{l" + "rr" * ncol + "}")
    lines.append(r"    \toprule")

    # Grouped header: one (Time, Mem) pair per solver.
    def label(s):
        return SOLVER_LABEL.get(s) or s

    grp = " & ".join(r"\multicolumn{2}{c}{" + label(s) + "}"
                     for s in solvers)
    lines.append(r"    & " + grp + r" \\")
    cmid = "".join(r"\cmidrule(lr){%d-%d}" % (2 + 2 * i, 3 + 2 * i)
                   for i in range(ncol))
    lines.append("    " + cmid)
    sub = " & ".join([r"Time [ms] & Mem [MiB]"] * ncol)
    lines.append(r"    Benchmark & " + sub + r" \\")
    lines.append(r"    \midrule")

    # Accumulate per-solver means for a summary row.
    acc_t = {s: [] for s in solvers}
    acc_m = {s: [] for s in solvers}

    for b in benches:
        cells = [_tex_escape(b)]
        for s in solvers:
            row = by.get((b, s))
            if row and row["n_ok"]:
                t_med = row["solve_ms"]["median"]
                t_std = row["solve_ms"]["std"]
                m_med = row["rss_bytes"]["median"]
                acc_t[s].append(t_med)
                acc_m[s].append(m_med)
                cells.append(rf"${t_med:.2f} \pm {t_std:.2f}$")
                cells.append(rf"{m_med / MIB:.0f}")
            else:
                cells.append("--")
                cells.append("--")
        lines.append("    " + " & ".join(cells) + r" \\")

    # Mean-across-benchmarks summary row.
    lines.append(r"    \midrule")
    mean_cells = [r"\textit{mean}"]
    for s in solvers:
        if acc_t[s]:
            mean_cells.append(rf"${statistics.mean(acc_t[s]):.2f}$")
            mean_cells.append(rf"{statistics.mean(acc_m[s]) / MIB:.0f}")
        else:
            mean_cells.append("--")
            mean_cells.append("--")
    lines.append("    " + " & ".join(mean_cells) + r" \\")

    lines.append(r"    \bottomrule")
    lines.append(r"  \end{tabular}")
    lines.append(r"\end{table}")
    return "\n".join(lines)


# --- Main --------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="LLTA ILP solver benchmark harness")
    ap.add_argument("--reps", type=int, default=10,
                    help="timed repetitions per (benchmark, solver) [10]")
    ap.add_argument("--warmup", type=int, default=1,
                    help="discarded warmup runs before timing [1]")
    ap.add_argument("--solvers", default="gurobi,highs",
                    help="comma list of solvers [gurobi,highs]")
    ap.add_argument("--benchmarks", default="",
                    help="comma list of benchmark names [all Maelardalen]")
    ap.add_argument("--skip-build", action="store_true",
                    help="reuse existing build_<name>/ artifacts, skip make")
    ap.add_argument("--build-timeout", type=int, default=180,
                    help="per-benchmark build timeout, seconds [180]")
    ap.add_argument("--run-timeout", type=int, default=300,
                    help="per-run analysis timeout, seconds [300]")
    ap.add_argument("--out-dir", default=HERE,
                    help="directory for the CSV outputs [tests/]")
    ap.add_argument("--report-only", action="store_true",
                    help="skip all runs; regenerate the LaTeX table from an "
                         "existing summary CSV in --out-dir")
    args = ap.parse_args()

    # Reporting-only path: rebuild the table from a prior sweep's summary CSV.
    if args.report_only:
        sum_path = os.path.join(args.out_dir, "solver_benchmark_summary.csv")
        if not os.path.isfile(sum_path):
            sys.exit(f"no summary CSV at {sum_path} -- run a sweep first")
        summary_rows, solvers = load_summary_csv(sum_path)
        tex = latex_table(summary_rows, solvers)
        tex_path = os.path.join(args.out_dir, "solver_benchmark_table.tex")
        with open(tex_path, "w") as f:
            f.write(tex + "\n")
        print(f"LaTeX table (also written to {tex_path}):\n")
        print(tex)
        return

    if not os.path.isfile(LLTA):
        sys.exit(f"llta not found at {LLTA} -- run ./config.sh build first")
    if not shutil.which("/usr/bin/time") and not os.path.exists("/usr/bin/time"):
        sys.exit("/usr/bin/time not found -- required for memory measurement")

    solvers = [s.strip() for s in args.solvers.split(",") if s.strip()]
    benches = ([b.strip() for b in args.benchmarks.split(",") if b.strip()]
               or all_benchmarks())

    print(f"llta:      {LLTA}")
    print(f"platform:  {platform.system()} ({'time -l' if IS_MACOS else 'time -v'})")
    print(f"solvers:   {', '.join(solvers)}")
    print(f"reps:      {args.reps} (+{args.warmup} warmup)")
    print(f"benchmarks: {len(benches)}")
    print("=" * 70)

    raw_rows = []      # per rep
    summary_rows = []  # per (benchmark, solver)
    skipped = []       # build-failed benchmarks
    # benchmark -> solver -> wcet (for agreement check)
    wcet_seen = {}

    for name in benches:
        # 1. Ensure artifacts exist.
        if not args.skip_build:
            ok = build(name, args.build_timeout)
        else:
            ok = artifacts_present(name)
        if not ok:
            print(f"SKIP  {name:16s} (build-fail / missing artifacts)")
            skipped.append(name)
            continue

        wcet_seen[name] = {}
        for solver in solvers:
            # 2. Warmup (discarded).
            for _ in range(max(0, args.warmup)):
                run_once(name, solver, args.run_timeout)

            # 3. Timed reps.
            solve_ms, rss, wcets, n_ok = [], [], set(), 0
            for rep in range(args.reps):
                r = run_once(name, solver, args.run_timeout)
                raw_rows.append([name, solver, rep,
                                 int(r["ok"]),
                                 r["wcet"] if r["wcet"] is not None else "",
                                 _fmt(r["solve_time_ms"]),
                                 r["peak_rss_bytes"]
                                 if r["peak_rss_bytes"] is not None else ""])
                if r["solve_time_ms"] is not None:
                    solve_ms.append(r["solve_time_ms"])
                if r["peak_rss_bytes"] is not None:
                    rss.append(r["peak_rss_bytes"])
                if r["wcet"] is not None:
                    wcets.add(r["wcet"])
                if r["ok"]:
                    n_ok += 1

            wcet_val = (next(iter(wcets)) if len(wcets) == 1
                        else (sorted(wcets) if wcets else None))
            if n_ok:
                wcet_seen[name][solver] = wcet_val

            sm = _stats(solve_ms)
            rm = _stats([float(x) for x in rss])
            summary_rows.append({
                "benchmark": name, "solver": solver,
                "n_ok": n_ok, "n_total": args.reps,
                "wcet": wcet_val,
                "solve_ms": sm, "rss_bytes": rm,
            })

            status = (f"WCET={wcet_val}" if n_ok else "SOLVE-FAIL")
            print(f"  {name:16s} {solver:7s} "
                  f"solve_med={_fmt(sm['median'])}ms "
                  f"rss_med={(rm['median']/1e6 if rm['median'] else 0):.1f}MB "
                  f"[{n_ok}/{args.reps}] {status}")

    # 4. Write CSVs.
    raw_path = os.path.join(args.out_dir, "solver_benchmark_raw.csv")
    with open(raw_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["benchmark", "solver", "rep", "ok", "wcet",
                    "solve_time_ms", "peak_rss_bytes"])
        w.writerows(raw_rows)

    sum_path = os.path.join(args.out_dir, "solver_benchmark_summary.csv")
    with open(sum_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "benchmark", "solver", "n_ok", "n_total", "wcet",
            "solve_ms_mean", "solve_ms_median", "solve_ms_std",
            "solve_ms_min", "solve_ms_max",
            "rss_bytes_mean", "rss_bytes_median", "rss_bytes_std",
            "rss_bytes_min", "rss_bytes_max",
        ])
        for s in summary_rows:
            sm, rm = s["solve_ms"], s["rss_bytes"]
            w.writerow([
                s["benchmark"], s["solver"], s["n_ok"], s["n_total"], s["wcet"],
                _fmt(sm["mean"]), _fmt(sm["median"]), _fmt(sm["std"]),
                _fmt(sm["min"]), _fmt(sm["max"]),
                _fmt(rm["mean"], 0), _fmt(rm["median"], 0), _fmt(rm["std"], 0),
                _fmt(rm["min"], 0), _fmt(rm["max"], 0),
            ])

    # 5. Console summary.
    print("=" * 70)
    print(f"raw CSV:     {raw_path}")
    print(f"summary CSV: {sum_path}")
    if skipped:
        print(f"\nskipped ({len(skipped)} build-fail): {', '.join(skipped)}")

    # WCET-agreement check across solvers (correctness, not perf).
    if len(solvers) > 1:
        disagree, no_result = [], []
        for name, per in wcet_seen.items():
            produced = {s: v for s, v in per.items() if v is not None}
            if len(produced) < len(solvers):
                no_result.append(name)
            vals = set(str(v) for v in produced.values())
            if len(vals) > 1:
                disagree.append((name, produced))
        if disagree:
            print("\n*** WCET DISAGREEMENT between solvers (correctness bug!) ***")
            for name, per in disagree:
                print(f"    {name}: " +
                      ", ".join(f"{s}={v}" for s, v in per.items()))
        if no_result:
            print(f"\nsolver(s) produced no WCET for: {', '.join(no_result)}")
            print("(these benchmarks could not be compared across all solvers)")
        if not disagree and not no_result:
            print("\nAll solvers agree on WCET for every benchmark.")

    # 6. LaTeX table (copy-pastable) -> stdout + .tex file.
    if summary_rows:
        tex = latex_table(summary_rows, solvers)
        tex_path = os.path.join(args.out_dir, "solver_benchmark_table.tex")
        with open(tex_path, "w") as f:
            f.write(tex + "\n")
        print("\n" + "=" * 70)
        print(f"LaTeX table (also written to {tex_path}):\n")
        print(tex)


if __name__ == "__main__":
    main()
