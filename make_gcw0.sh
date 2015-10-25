#!/bin/sh

# To create a debug build, run `cmake -D CMAKE_BUILD_TYPE=Debug .` instead

# For more info: http://github.com/cxong/cdogs-sdl/wiki/Developer-Getting-Started:-GCW-Zero
rm CMakeCache.txt
rm gcw0build -rf
mkdir gcw0build
cd gcw0build
cmake -DCMAKE_TOOLCHAIN_FILE="/opt/gcw0-toolchain/usr/share/buildroot/toolchainfile.cmake" ..
make
