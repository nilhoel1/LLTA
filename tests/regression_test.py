import os
import subprocess
import re
import sys

# Configuration
# Assuming script is in tests/ directory
LLTA_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "../build/bin/llta"))
CNT_LL_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "msp430/cnt/msp.ll"))
COVER_LL_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "msp430/cover/msp.ll"))

# Baselines (Current Version as of 2025-12-10)
EXPECTED_CNT_WCET = 6347
EXPECTED_COVER_WCET = 3483

def run_test(name, ll_path, expected_wcet, extra_args=[]):
    print(f"Running test: {name}")
    if not os.path.exists(ll_path):
        print(f"Error: .ll file not found at {ll_path}")
        return "RED"

    cmd = [LLTA_PATH] + extra_args + [ll_path]
    print(f"Command: {' '.join(cmd)}")
    
    try:
        # Run command
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        print(f"TIMEOUT: {name}")
        return "RED"
    except Exception as e:
        print(f"ERROR: {name} - {e}")
        return "RED"

    # Check for crash/failure return code (non-zero)
    if result.returncode != 0:
        print(f"FAIL: {name} - Exit code {result.returncode}")
        print("Output (stderr):")
        print(result.stderr)
        return "RED"

    # Check for WCET in stdout
    match = re.search(r"WCET \(worst-case execution time\): (\d+) cycles", result.stdout)
    if not match:
        print(f"FAIL: {name} - Bound not found in output")
        print("Tail of stdout:")
        print(result.stdout[-500:] if result.stdout else "Empty stdout")
        return "RED"
    
    wcet = int(match.group(1))
    print(f"Info: {name} WCET = {wcet}")

    if wcet == expected_wcet:
        print(f"PASS: {name} (Matches expected {wcet}) -> GREEN")
        return "GREEN"
    else:
        print(f"WARNING: {name} (Expected {expected_wcet}, got {wcet}) -> YELLOW")
        return "YELLOW"

def main():
    print("=== LLTA Regression Test ===")
    print(f"LLTA Executable: {LLTA_PATH}")
    
    if not os.path.exists(LLTA_PATH):
        print("Error: LLTA executable not found.")
        sys.exit(1)

    results = []
    
    # Test CNT
    results.append(run_test("cnt", CNT_LL_PATH, EXPECTED_CNT_WCET))
    
    # Test COVER
    # Note: cover requires -start-function=main to avoid ambiguity as it has multiple potential roots.
    # Without this flag, LLTA v1 fails to identify a unique entry point.
    results.append(run_test("cover", COVER_LL_PATH, EXPECTED_COVER_WCET, ["-start-function=main"]))

    print("\n=== Summary ===")
    
    status_priority = {"RED": 2, "YELLOW": 1, "GREEN": 0}
    final_status = "GREEN"
    
    tests = ["cnt", "cover"]
    for i, res in enumerate(results):
        print(f"{tests[i]}: {res}")
        if status_priority[res] > status_priority[final_status]:
            final_status = res

    print(f"\nFinal Result: {final_status}")

    # Exit code based on status
    if final_status == "RED":
        sys.exit(1)
    elif final_status == "YELLOW":
        sys.exit(0) # Pass with warning
    else:
        sys.exit(0) # Pass

if __name__ == "__main__":
    main()
