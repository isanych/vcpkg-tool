#!/bin/bash

d=$(X= cd -- "$(dirname -- "$0")" && pwd -P)
export CC=`which gcc`
export CXX=`which g++`
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/opt/vcpkg/installed/x64l/lib"
cmake "$d" -Wno-dev -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64l -GNinja -DCMAKE_BUILD_TYPE=Release "$@"
ninja
