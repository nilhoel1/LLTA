#!/usr/bin/env bash
#
# Test script for LoopBoundPlugin
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
PLUGIN="$BUILD_DIR/lib/LoopBoundPlugin.dylib"
if [ ! -f "$PLUGIN" ]; then
    PLUGIN="$BUILD_DIR/lib/LoopBoundPlugin.so"
fi
CLANG="$BUILD_DIR/bin/clang"
EXAMPLE_FILE="$SCRIPT_DIR/examples/loop_bound_example.c"
EXAMPLE_DIR="$SCRIPT_DIR/examples"
JSON_FILE="$EXAMPLE_DIR/loop_bound_example.c.loop_bounds.json"

# Cleanup function
cleanup() {
    echo "Cleaning up generated files..."
    rm -f "$EXAMPLE_DIR"/*.json
    rm -f "$EXAMPLE_DIR"/*.ll
    rm -f "$EXAMPLE_DIR"/*.o
    rm -f "$EXAMPLE_DIR"/*.S
    echo "Cleanup complete"
}

# Set trap to cleanup on exit
trap cleanup EXIT

# Check if plugin exists
if [ ! -f "$PLUGIN" ]; then
    echo -e "${RED}Error: Plugin not found at $PLUGIN${NC}"
    echo "Please build the plugin first with: ninja LoopBoundPlugin"
    exit 1
fi

# Check if clang exists
if [ ! -f "$CLANG" ]; then
    echo -e "${RED}Error: Clang not found at $CLANG${NC}"
    echo "Please build clang first"
    exit 1
fi

# Check if example file exists
if [ ! -f "$EXAMPLE_FILE" ]; then
    echo -e "${RED}Error: Example file not found at $EXAMPLE_FILE${NC}"
    exit 1
fi

echo "========================================"
echo "  LoopBoundPlugin Test Suite"
echo "========================================"
echo ""

# Test 1: Plugin loads
echo -e "${YELLOW}Test 1: Plugin loads correctly${NC}"
if "$CLANG" -cc1 -triple msp430 -load "$PLUGIN" -plugin loop-bound "$EXAMPLE_FILE" 2>&1 | grep -q "error"; then
    echo -e "${RED}✗ FAIL: Plugin failed to load or process file${NC}"
    exit 1
else
    echo -e "${GREEN}✓ PASS: Plugin loads successfully${NC}"
fi
echo ""

# Test 2: Pragma parsing with verbose output
echo -e "${YELLOW}Test 2: Pragma parsing (verbose mode)${NC}"
OUTPUT=$("$CLANG" -cc1 -triple msp430 -load "$PLUGIN" -plugin loop-bound -plugin-arg-loop-bound verbose "$EXAMPLE_FILE" 2>&1)

# Check for expected pragmas
PRAGMA_COUNT=$(echo "$OUTPUT" | grep -c "Stored loop_bound pragma" || true)
if [ "$PRAGMA_COUNT" -ge 4 ]; then
    echo -e "${GREEN}✓ PASS: Found $PRAGMA_COUNT pragmas${NC}"
else
    echo -e "${RED}✗ FAIL: Expected at least 4 pragmas, found $PRAGMA_COUNT${NC}"
    exit 1
fi
echo ""

# Test 3: Loop association
echo -e "${YELLOW}Test 3: Loop-pragma association${NC}"
LOOP_COUNT=$(echo "$OUTPUT" | grep -c "Attaching loop bounds" || true)
if [ "$LOOP_COUNT" -ge 4 ]; then
    echo -e "${GREEN}✓ PASS: Found $LOOP_COUNT loop associations${NC}"
else
    echo -e "${RED}✗ FAIL: Expected at least 4 loop associations, found $LOOP_COUNT${NC}"
    exit 1
fi
echo ""

# Test 4: Silent mode (default)
echo -e "${YELLOW}Test 4: Silent mode (default behavior)${NC}"
OUTPUT=$("$CLANG" -cc1 -triple msp430 -load "$PLUGIN" -plugin loop-bound "$EXAMPLE_FILE" 2>&1)
if echo "$OUTPUT" | grep -q "LoopBoundPlugin"; then
    echo -e "${RED}✗ FAIL: Plugin should run silently by default${NC}"
    exit 1
else
    echo -e "${GREEN}✓ PASS: Plugin runs silently${NC}"
fi
echo ""

# Test 5: Help message
echo -e "${YELLOW}Test 5: Help message${NC}"
OUTPUT=$("$CLANG" -cc1 -triple msp430 -load "$PLUGIN" -plugin loop-bound -plugin-arg-loop-bound help "$EXAMPLE_FILE" 2>&1)
if echo "$OUTPUT" | grep -q "LoopBoundPlugin usage"; then
    echo -e "${GREEN}✓ PASS: Help message displayed${NC}"
else
    echo -e "${RED}✗ FAIL: Help message not displayed${NC}"
    exit 1
fi
echo ""

# Test 6: JSON file generation
echo -e "${YELLOW}Test 6: JSON file generation${NC}"
# Remove old JSON file if it exists
rm -f "$JSON_FILE"
"$CLANG" -cc1 -triple msp430 -load "$PLUGIN" -plugin loop-bound "$EXAMPLE_FILE" 2>&1 > /dev/null
if [ -f "$JSON_FILE" ]; then
    echo -e "${GREEN}✓ PASS: JSON file generated${NC}"
else
    echo -e "${RED}✗ FAIL: JSON file not generated${NC}"
    exit 1
fi
echo ""

# Test 7: JSON content validation
echo -e "${YELLOW}Test 7: JSON content validation${NC}"
# Check if JSON is valid
if ! python3 -m json.tool "$JSON_FILE" > /dev/null 2>&1; then
    echo -e "${RED}✗ FAIL: JSON file is not valid${NC}"
    exit 1
fi

# Check for expected structure and values
JSON_CONTENT=$(cat "$JSON_FILE")
LOOP_BOUNDS_COUNT=$(echo "$JSON_CONTENT" | grep -o '"file"' | wc -l | tr -d ' ')
if [ "$LOOP_BOUNDS_COUNT" -ge 4 ]; then
    echo -e "${GREEN}✓ PASS: JSON contains $LOOP_BOUNDS_COUNT loop bound entries${NC}"
else
    echo -e "${RED}✗ FAIL: Expected at least 4 loop bounds, found $LOOP_BOUNDS_COUNT${NC}"
    exit 1
fi

# Validate specific fields exist
if echo "$JSON_CONTENT" | grep -q '"loop_bounds"' && \
   echo "$JSON_CONTENT" | grep -q '"line"' && \
   echo "$JSON_CONTENT" | grep -q '"column"' && \
   echo "$JSON_CONTENT" | grep -q '"lower_bound"' && \
   echo "$JSON_CONTENT" | grep -q '"upper_bound"'; then
    echo -e "${GREEN}✓ PASS: JSON has correct structure (loop_bounds, line, column, lower_bound, upper_bound)${NC}"
else
    echo -e "${RED}✗ FAIL: JSON missing required fields${NC}"
    exit 1
fi

# Check for expected loop bounds values (1, 30) and (1, 40)
if echo "$JSON_CONTENT" | grep -q '"lower_bound": 1' && \
   echo "$JSON_CONTENT" | grep -q '"upper_bound": 30' && \
   echo "$JSON_CONTENT" | grep -q '"upper_bound": 40'; then
    echo -e "${GREEN}✓ PASS: JSON contains expected bound values (1,30) and (1,40)${NC}"
else
    echo -e "${RED}✗ FAIL: JSON does not contain expected bound values${NC}"
    exit 1
fi
echo ""

echo "========================================"
echo -e "${GREEN}All tests passed!${NC}"
echo "========================================"
