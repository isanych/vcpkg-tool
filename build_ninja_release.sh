#!/bin/bash

d=$(X= cd -- "$(dirname -- "$0")" && pwd -P)
export CC=`which gcc`
export CXX=`which g++`
VCPKG_VERSION=$(git describe --match v0)
VCPKG_VERSION=${VCPKG_VERSION#v0-}
cmake "$d" -Wno-author -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT:-/opt/vcpkg}/scripts/buildsystems/vcpkg.cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DVCPKG_VERSION="$VCPKG_VERSION" "$@"
ninja
ninja test
strip vcpkg
./vcpkg --version
if [[ "$ART_PUBLISH" != 0 ]]; then
  vcpkgHostArch="$(uname -m)"
  case "$vcpkgHostArch" in
    x86_64|amd64) vcpkgToolArch=x64l ;;
    aarch64|arm64) vcpkgToolArch=a64l ;;
    *)
      echo "Unsupported Linux architecture: $vcpkgHostArch" >&2
      exit 1
      ;;
  esac
  tar czf vcpkg-tool-2026.4-${vcpkgToolArch}.tgz vcpkg.exe
  artc put vcpkg-tool-2026.4-${vcpkgToolArch}.tgz /vcpkg-tool/
fi
