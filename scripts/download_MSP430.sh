#!/usr/bin/env bash

set -e

# Get script directory and workspace root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

EXTERNAL_DEPS_DIR="${WORKSPACE_ROOT}/externalDeps"
MSP430_DIR="${EXTERNAL_DEPS_DIR}/MSP430"

SUPPORT_FILES_URL="https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-LlCjWuAbzH/9.3.1.2/msp430-gcc-support-files-1.212.zip"
SUPPORT_FILES_ZIP="msp430-gcc-support-files-1.212.zip"
SUPPORT_FILES_DIR="msp430-gcc-support-files"

DRIVERLIB_URL="https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430_Driver_Library/latest/exports/msp430_driverlib_2_91_13_01.zip"
DRIVERLIB_ZIP="msp430_driverlib_2_91_13_01.zip"
DRIVERLIB_DIR="msp430-driverlib"

# Determine OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    GCC_URL="https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-LlCjWuAbzH/9.3.1.2/msp430-gcc-9.3.1.11_macos.tar.bz2"
    GCC_TAR="msp430-gcc-9.3.1.11_macos.tar.bz2"
    GCC_EXTRACTED_DIR="msp430-gcc-9.3.1.11_macos"
else
    GCC_URL="https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-LlCjWuAbzH/9.3.1.2/msp430-gcc-9.3.1.11_linux64.tar.bz2"
    GCC_TAR="msp430-gcc-9.3.1.11_linux64.tar.bz2"
    GCC_EXTRACTED_DIR="msp430-gcc-9.3.1.11_linux64"
fi
GCC_TARGET_DIR="msp430-gcc"

# Create MSP430 directory
mkdir -p "$MSP430_DIR"
cd "$MSP430_DIR"

echo "Setting up MSP430 Toolchain in $MSP430_DIR..."

# 1. Support Files
if [ -d "$SUPPORT_FILES_DIR" ]; then
    echo "Support files already exist."
else
    echo "Downloading Support Files..."
    rm -rf "$SUPPORT_FILES_DIR"
    curl -L -O "$SUPPORT_FILES_URL"
    unzip -q "$SUPPORT_FILES_ZIP"
    rm "$SUPPORT_FILES_ZIP"
fi

# 2. DriverLib
if [ -d "$DRIVERLIB_DIR" ]; then
    echo "DriverLib already exists."
else
    echo "Downloading DriverLib..."
    rm -rf "$DRIVERLIB_DIR"
    curl -L -O "$DRIVERLIB_URL"
    unzip -q "$DRIVERLIB_ZIP"
    mv "msp430_driverlib_2_91_13_01" "$DRIVERLIB_DIR"
    rm "$DRIVERLIB_ZIP"
fi

# 3. GCC Toolchain
if [ -d "$GCC_TARGET_DIR" ]; then
    echo "MSP430 GCC Toolchain already exists."
else
    echo "Downloading MSP430 GCC Toolchain..."
    rm -rf "$GCC_TARGET_DIR"
    curl -L -O "$GCC_URL"
    tar -xjf "$GCC_TAR"
    mv "$GCC_EXTRACTED_DIR" "$GCC_TARGET_DIR"
    rm "$GCC_TAR"
fi

echo "MSP430 Toolchain setup complete."