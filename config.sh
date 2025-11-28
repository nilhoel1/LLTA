#!/bin/bash

set -e

LLVM_VERSION="20.1.8"
LLVM_PROJECT_DIR="externalDeps/llvm-project-${LLVM_VERSION}.src"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

download() {
  echo "Downloading LLVM ${LLVM_VERSION}..."
  "${SCRIPT_DIR}/scripts/download_llvm.sh"
  echo "LLVM downloaded and extracted."
}

apply_patch() {
  echo "Applying patches..."
  "${SCRIPT_DIR}/scripts/create_llvm_patch.sh" apply
  echo "Patches applied."
}

conf() {
  # Check if LLVM project exists
  if [ ! -d "$LLVM_PROJECT_DIR" ]; then
    echo "LLVM project not found. Downloading..."
    download
  fi

  # Check if patch exists and apply it
  PATCH_FILE="${SCRIPT_DIR}/scripts/llvm-${LLVM_VERSION}-custom.patch"
  PATCH_APPLIED_MARKER="${LLVM_PROJECT_DIR}/.patch_applied"

  if [ -f "$PATCH_FILE" ]; then
    if [ ! -f "$PATCH_APPLIED_MARKER" ]; then
      echo "Patch file found. Applying..."
      apply_patch
      touch "$PATCH_APPLIED_MARKER"
    else
      echo "Patch already applied. Skipping."
    fi
  fi

  echo "Configuring LLVM build..."
  CC=clang CXX=clang++ cmake \
    -S "${LLVM_PROJECT_DIR}/llvm" \
    -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_TARGETS_TO_BUILD='MSP430' \
    -DLLVM_EXTERNAL_LLTA_SOURCE_DIR="${SCRIPT_DIR}" \
    -DLLVM_EXTERNAL_CLANG_PLUGIN_SOURCE_DIR="${SCRIPT_DIR}/clang-plugin" \
    -DLLVM_EXTERNAL_PROJECTS='LLTA;clang-plugin' \
    -DLLVM_ENABLE_PROJECTS='clang' \
    -DCLANG_SOURCE_DIR="${LLVM_PROJECT_DIR}/clang" \
    -DLLVM_USE_LINKER=lld \
    -GNinja
  cp build/compile_commands.json .
  echo "Configuration complete."
}

build() {
  if [ ! -d "build" ]; then
    echo "No build folder found! Run 'config' first."
    exit 1
  fi
  cd build
  ninja llta LoopBoundPlugin
  cd ..
}

build_all() {
  if [ ! -d "build" ]; then
    echo "No build folder found! Run 'config' first."
    exit 1
  fi
  cd build
  ninja
  cd ..
}

clean() {
  echo "Cleaning build artifacts..."
  rm -rf build
  rm -f compile_commands.json
  echo "Clean complete."
}

mrproper() {
  echo "Performing mrproper (deep clean)..."
  clean
  echo "Removing contents of externalDeps..."
  if [ -d "externalDeps" ]; then
    find externalDeps -mindepth 1 -maxdepth 1 -not -name '.gitignore' -exec rm -rf {} +
  fi
  echo "MrProper complete."
}

case $1 in
download | d)
  download
  ;;
patch | p)
  apply_patch
  ;;
config | c)
  conf
  ;;
build | b)
  build
  ;;
buildall | build-all | ba)
  build_all
  ;;
clean)
  clean
  ;;
mrproper | distclean)
  mrproper
  ;;
*)
  if [ $1 ]; then
    echo "Unknown argument: $1"
  fi
  echo "Script to configure and build:"
  echo "  d | download               Download LLVM ${LLVM_VERSION} source."
  echo "  p | patch                  Apply custom patches."
  echo "  c | config                 Configure for Development (auto-downloads if needed)."
  echo "  b | build                  Build the llta target."
  echo "  ba| build-all              Build all targets."
  echo "  clean                      Remove build artifacts."
  echo "  mrproper | distclean       Deep clean (removes build + externalDeps)."
  exit
  ;;
esac
