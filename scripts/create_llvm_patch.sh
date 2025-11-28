#!/usr/bin/env bash

set -e

# Get script directory and workspace root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Configuration
LLVM_VERSION="20.1.8"
EXTERNAL_DEPS_DIR="${WORKSPACE_ROOT}/externalDeps"
ORIGINAL_DIR="${EXTERNAL_DEPS_DIR}/llvm-project-${LLVM_VERSION}.src"
MODIFIED_DIR="${EXTERNAL_DEPS_DIR}/llvm-project-${LLVM_VERSION}.src.patch"
PATCH_FILE="${SCRIPT_DIR}/llvm-${LLVM_VERSION}-custom.patch"

# Default action
ACTION="${1:-apply}"

show_usage() {
    echo "Usage: $0 [apply|create]"
    echo ""
    echo "  apply   - Apply patch to modified directory (default)"
    echo "  create  - Generate patch from original vs modified"
    echo ""
}

create_patch() {
    if [ ! -d "$ORIGINAL_DIR" ]; then
        echo "Error: Original directory not found: $ORIGINAL_DIR"
        exit 1
    fi

    if [ ! -d "$MODIFIED_DIR" ]; then
        echo "Error: Modified directory not found: $MODIFIED_DIR"
        exit 1
    fi

    echo "Generating patch..."
    cd "$EXTERNAL_DEPS_DIR"

    TEMP_PATCH="/tmp/llvm-${LLVM_VERSION}-custom.patch"

    diff -ruN \
        "$(basename "$ORIGINAL_DIR")" \
        "$(basename "$MODIFIED_DIR")" \
        > "$TEMP_PATCH" || true

    if [ -f "$TEMP_PATCH" ] && [ -s "$TEMP_PATCH" ]; then
        mv "$TEMP_PATCH" "$PATCH_FILE"
        echo "✓ Patch created: $PATCH_FILE"
    else
        echo "No differences found."
        rm -f "$TEMP_PATCH"
    fi
}

apply_patch() {
    if [ ! -f "$PATCH_FILE" ]; then
        echo "Error: Patch file not found: $PATCH_FILE"
        exit 1
    fi

    if [ ! -d "$ORIGINAL_DIR" ]; then
        echo "Error: Directory not found: $ORIGINAL_DIR"
        exit 1
    fi

    echo "Applying patch from: $PATCH_FILE"
    cd "$EXTERNAL_DEPS_DIR"
    patch -p0 -s < "$PATCH_FILE"
    echo "✓ Done"
}

case "$ACTION" in
    apply)
        apply_patch
        ;;
    create)
        create_patch
        ;;
    -h|--help)
        show_usage
        exit 0
        ;;
    *)
        echo "Error: Unknown action '$ACTION'"
        echo ""
        show_usage
        exit 1
        ;;
esac
