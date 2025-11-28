#!/usr/bin/env bash

set -e

# Get script directory and workspace root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# HiGHS version to download
HIGHS_VERSION="1.12.0"
EXTERNAL_DEPS_DIR="${WORKSPACE_ROOT}/externalDeps"
SOURCE_ARCHIVE="v${HIGHS_VERSION}.tar.gz"
SOURCE_URL="https://github.com/ERGO-Code/HiGHS/archive/refs/tags/${SOURCE_ARCHIVE}"

# Create externalDeps directory if it doesn't exist
mkdir -p "$EXTERNAL_DEPS_DIR"
cd "$EXTERNAL_DEPS_DIR"

# Download source
echo "Downloading HiGHS ${HIGHS_VERSION} source..."
if [ -f "$SOURCE_ARCHIVE" ]; then
    echo "Source archive already exists: $SOURCE_ARCHIVE"
    read -p "Re-download? (y/N): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        rm "$SOURCE_ARCHIVE"
        curl -L -O "$SOURCE_URL"
    fi
else
    curl -L -O "$SOURCE_URL"
fi

# Extract source
if [ -d "HiGHS-${HIGHS_VERSION}" ]; then
    echo "HiGHS source already extracted"
    read -p "Re-extract? (y/N): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        rm -rf "HiGHS-${HIGHS_VERSION}"
        tar -xzf "$SOURCE_ARCHIVE"
    fi
else
    echo "Extracting source..."
    tar -xzf "$SOURCE_ARCHIVE"
fi

# Build HiGHS
echo "Building HiGHS from source..."
cd "HiGHS-${HIGHS_VERSION}"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${EXTERNAL_DEPS_DIR}/highs"
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make install
cd ../..

echo "Done! HiGHS ${HIGHS_VERSION} built and installed to ${EXTERNAL_DEPS_DIR}/highs"

# Clean up archive
echo "Cleaning up archive..."
rm -f "$SOURCE_ARCHIVE"
echo "Archive removed."
