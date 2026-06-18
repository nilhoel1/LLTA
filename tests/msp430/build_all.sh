#!/usr/bin/env bash
#
# build_all.sh — build (and optionally analyze) every Maelardalen benchmark.
#
# Drives the modular Makefile once per source in ../srcMaelardalen, producing a
# .opt.ll and a .dump for each. With "analyze" it also runs the full LLTA WCET
# analysis (loop bounds via the Clang LoopBoundPlugin + dump-file resolution).
#
# Usage:
#   ./build_all.sh            # build .ll + .dump for every benchmark
#   ./build_all.sh analyze    # also run WCET analysis
#
# Per-benchmark output is captured in build_<name>/build.log. One benchmark
# failing never aborts the run; the script always exits 0 (it is a generator /
# reporter, not a regression gate — see regression_test.py for the gate).
#
# Env: BENCH_TIMEOUT (seconds, default 120) bounds each make invocation.

set -u
cd "$(dirname "$0")" || exit 1

SRC_DIR="../srcMaelardalen"
TIMEOUT="${BENCH_TIMEOUT:-120}"

DO_ANALYZE=0
[ "${1:-}" = "analyze" ] && DO_ANALYZE=1

benchmarks=$(for f in "$SRC_DIR"/*.c; do basename "$f" .c; done | sort)

# Count loop_bounds entries in a plugin JSON (0 on any error / missing file).
bounds_count() {
    python3 -c "import json,sys; print(len(json.load(open(sys.argv[1])).get('loop_bounds',[])))" "$1" 2>/dev/null || echo 0
}

# Extract the last WCET value from an analysis log (empty if none).
wcet_value() {
    grep -oE '(WCET \(worst-case execution time\): |All solvers agree on WCET: )[0-9]+' "$1" 2>/dev/null \
        | grep -oE '[0-9]+$' | tail -1
}

printf "%-16s %-7s %-12s %s\n" "BENCHMARK" "BUILD" "BOUNDS" "ANALYZE"
printf -- '-%.0s' $(seq 1 56); echo

n_build_ok=0 n_wcet_ok=0 n_total=0
for b in $benchmarks; do
    n_total=$((n_total + 1))
    mkdir -p "build_$b"
    log="build_$b/build.log"
    : > "$log"

    build="fail"; bounds="-"; analyze="-"

    timeout "$TIMEOUT" make TEST="$b" all >>"$log" 2>&1
    rc=$?
    if [ "$rc" -eq 0 ]; then
        build="ok"; n_build_ok=$((n_build_ok + 1))
    elif [ "$rc" -eq 124 ]; then
        build="timeout"
    fi

    if [ "$DO_ANALYZE" -eq 1 ] && [ "$build" = "ok" ]; then
        timeout "$TIMEOUT" make TEST="$b" analyze >>"$log" 2>&1
        arc=$?

        j="build_$b/$b.loop_bounds.json"
        if [ -f "$j" ]; then
            n=$(bounds_count "$j")
            [ "$n" = "0" ] && bounds="empty" || bounds="$n loops"
        fi

        if [ "$arc" -eq 124 ]; then
            analyze="timeout"
        else
            w=$(wcet_value "build_$b/$b.wcet")
            if [ -n "$w" ]; then
                analyze="wcet=$w"; n_wcet_ok=$((n_wcet_ok + 1))
            elif [ -f "build_$b/$b.wcet" ]; then
                analyze="fail"
            else
                analyze="none"
            fi
        fi
    fi

    printf "%-16s %-7s %-12s %s\n" "$b" "$build" "$bounds" "$analyze"
done

echo
echo "Built $n_build_ok/$n_total benchmarks."
[ "$DO_ANALYZE" -eq 1 ] && echo "Analyzed (WCET produced) $n_wcet_ok/$n_total benchmarks."
exit 0
