#!/usr/bin/env bash
#
# Generate loop bound JSON files from C source files using LoopBoundPlugin
#
# Usage: ./generate_loop_bounds.sh <arch> <target>
#   arch:   target architecture (e.g., msp430)
#   target: benchmark name (e.g., blink, cnt, cover)
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse command line arguments
if [ "$#" -ne 2 ]; then
    echo -e "${RED}Error: Invalid number of arguments${NC}"
    echo "Usage: $0 <arch> <target>"
    echo "  arch:   target architecture (e.g., msp430)"
    echo "  target: benchmark name (e.g., blink, cnt, cover)"
    echo ""
    echo "Example: $0 msp430 cnt"
    exit 1
fi

ARCH="$1"
TARGET="$2"

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_DIR="$SCRIPT_DIR/$ARCH/$TARGET"

# Check if target directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo -e "${RED}Error: Target directory not found: $TARGET_DIR${NC}"
    echo "Available targets for $ARCH:"
    if [ -d "$SCRIPT_DIR/$ARCH" ]; then
        ls -1 "$SCRIPT_DIR/$ARCH"
    else
        echo "  (architecture directory not found)"
    fi
    exit 1
fi

# Find plugin
PLUGIN="$BUILD_DIR/lib/LoopBoundPlugin.dylib"
if [ ! -f "$PLUGIN" ]; then
    PLUGIN="$BUILD_DIR/lib/LoopBoundPlugin.so"
fi

# Check if plugin exists
if [ ! -f "$PLUGIN" ]; then
    echo -e "${RED}Error: Plugin not found at $PLUGIN${NC}"
    echo "Please build the plugin first with: ninja LoopBoundPlugin"
    exit 1
fi

# Find clang
CLANG="$BUILD_DIR/bin/clang"
if [ ! -f "$CLANG" ]; then
    echo -e "${RED}Error: Clang not found at $CLANG${NC}"
    echo "Please build clang first"
    exit 1
fi

# Find all C files in target directory
C_FILES=$(find "$TARGET_DIR" -maxdepth 1 -name "*.c" -type f)

if [ -z "$C_FILES" ]; then
    echo -e "${YELLOW}Warning: No .c files found in $TARGET_DIR${NC}"
    exit 0
fi

echo "========================================"
echo "  Loop Bound JSON Generator"
echo "========================================"
echo -e "${BLUE}Target:${NC}       $TARGET"
echo -e "${BLUE}Architecture:${NC} $ARCH"
echo -e "${BLUE}Directory:${NC}    $TARGET_DIR"
echo "========================================"
echo ""

# Process each C file
FILE_COUNT=0
SUCCESS_COUNT=0
FAIL_COUNT=0

for C_FILE in $C_FILES; do
    FILE_COUNT=$((FILE_COUNT + 1))
    BASENAME=$(basename "$C_FILE")
    JSON_FILE="${C_FILE}.loop_bounds.json"

    echo -e "${BLUE}Processing:${NC} $BASENAME"

    # Remove old JSON file if it exists
    rm -f "$JSON_FILE"

    # Run clang with plugin
    if "$CLANG" -cc1 -triple "$ARCH" -load "$PLUGIN" -plugin loop-bound "$C_FILE" 2>&1 | grep -q "error"; then
        echo -e "${RED}  ✗ FAIL: Plugin failed to process file${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        continue
    fi

    # Check if JSON file was generated
    if [ -f "$JSON_FILE" ]; then
        # Validate JSON
        if python3 -m json.tool "$JSON_FILE" > /dev/null 2>&1; then
            LOOP_COUNT=$(grep -o '"file"' "$JSON_FILE" | wc -l | tr -d ' ')
            echo -e "${GREEN}  ✓ SUCCESS: Generated JSON with $LOOP_COUNT loop bound(s)${NC}"
            echo -e "  ${BLUE}Output:${NC} $(basename "$JSON_FILE")"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo -e "${RED}  ✗ FAIL: Invalid JSON generated${NC}"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    else
        echo -e "${YELLOW}  ⚠ No loop bounds found (no pragmas in source)${NC}"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    fi

    echo ""
done

echo "========================================"
echo "  Summary"
echo "========================================"
echo -e "${BLUE}Files processed:${NC} $FILE_COUNT"
echo -e "${GREEN}Successful:${NC}      $SUCCESS_COUNT"
echo -e "${RED}Failed:${NC}          $FAIL_COUNT"
echo "========================================"

if [ "$FAIL_COUNT" -gt 0 ]; then
    exit 1
fi

exit 0
