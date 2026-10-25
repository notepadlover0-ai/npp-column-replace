#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
cd "$root"

build_arch() {
    local arch="$1"
    local toolchain="$2"
    echo "==> Building $arch"
    cmake -S . -B "build/$arch" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "build/$arch" -j
}

build_arch x86_64 cmake/mingw-x86_64.cmake
build_arch x86 cmake/mingw-i686.cmake

echo
echo "Fertig:"
ls -lh build/x86_64/ColumnReplace.dll build/x86/ColumnReplace.dll
