#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"

echo "================================================"
echo "  Optimizer Optional Build Tests"
echo "================================================"
echo ""

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=8

# ==================== Gurobi Tests ====================

echo "--- GUROBI TESTS ---"
echo ""

# Test 1: Build with Gurobi
echo "[TEST 1/$TOTAL_TESTS] Building Gurobi example WITH support..."
rm -rf build_gurobi_with
mkdir -p build_gurobi_with
cd build_gurobi_with
if cmake .. -DENABLE_GUROBI=ON -DENABLE_HIGHS=OFF && make gurobi_optional_example; then
    echo "✓ PASS: Build with Gurobi succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Build with Gurobi failed"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Test 2: Run with Gurobi
echo "[TEST 2/$TOTAL_TESTS] Running Gurobi example with support..."
if ./build_gurobi_with/gurobi_optional_example; then
    echo "✓ PASS: Execution with Gurobi succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Execution with Gurobi failed"
    ((TESTS_FAILED++))
fi
echo ""

# Test 3: Build without Gurobi
echo "[TEST 3/$TOTAL_TESTS] Building Gurobi example WITHOUT support..."
rm -rf build_gurobi_without
mkdir -p build_gurobi_without
cd build_gurobi_without
if cmake .. -DENABLE_GUROBI=OFF -DENABLE_HIGHS=OFF && make gurobi_optional_example; then
    echo "✓ PASS: Build without Gurobi succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Build without Gurobi failed"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Test 4: Run without Gurobi
echo "[TEST 4/$TOTAL_TESTS] Running Gurobi example without support..."
if ./build_gurobi_without/gurobi_optional_example; then
    echo "✓ PASS: Execution without Gurobi succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Execution without Gurobi failed"
    ((TESTS_FAILED++))
fi
echo ""

# ==================== HiGHS Tests ====================

echo "--- HIGHS TESTS ---"
echo ""

# Test 5: Build with HiGHS
echo "[TEST 5/$TOTAL_TESTS] Building HiGHS example WITH support..."
rm -rf build_highs_with
mkdir -p build_highs_with
cd build_highs_with
if cmake .. -DENABLE_GUROBI=OFF -DENABLE_HIGHS=ON && make highs_optional_example; then
    echo "✓ PASS: Build with HiGHS succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Build with HiGHS failed"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Test 6: Run with HiGHS
echo "[TEST 6/$TOTAL_TESTS] Running HiGHS example with support..."
if ./build_highs_with/highs_optional_example; then
    echo "✓ PASS: Execution with HiGHS succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Execution with HiGHS failed"
    ((TESTS_FAILED++))
fi
echo ""

# Test 7: Build without HiGHS
echo "[TEST 7/$TOTAL_TESTS] Building HiGHS example WITHOUT support..."
rm -rf build_highs_without
mkdir -p build_highs_without
cd build_highs_without
if cmake .. -DENABLE_GUROBI=OFF -DENABLE_HIGHS=OFF && make highs_optional_example; then
    echo "✓ PASS: Build without HiGHS succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Build without HiGHS failed"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Test 8: Run without HiGHS
echo "[TEST 8/$TOTAL_TESTS] Running HiGHS example without support..."
if ./build_highs_without/highs_optional_example; then
    echo "✓ PASS: Execution without HiGHS succeeded"
    ((TESTS_PASSED++))
else
    echo "✗ FAIL: Execution without HiGHS failed"
    ((TESTS_FAILED++))
fi
echo ""

# Summary
echo "================================================"
echo "  Test Summary"
echo "================================================"
echo "Tests Passed: $TESTS_PASSED/$TOTAL_TESTS"
echo "Tests Failed: $TESTS_FAILED/$TOTAL_TESTS"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ ALL TESTS PASSED"
    echo ""
    echo "Cleanup: Removing build directories..."
    rm -rf build_gurobi_with build_gurobi_without build_highs_with build_highs_without
    exit 0
else
    echo "✗ SOME TESTS FAILED"
    exit 1
fi
