#!/bin/bash

d=$(X= cd -- "$(dirname -- "$0")" && pwd -P)
export CC=`which gcc`
export CXX=`which g++`
cmake "$d" -Wno-dev -GNinja -DCMAKE_BUILD_TYPE=Release "$@"
ninja
ninja test
