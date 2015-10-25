#!/bin/sh

TOOLCHAIN_DIR=$1
if [ "$1" = "" ]
then
    TOOLCHAIN_DIR=/opt
fi

# To create a debug build, run `cmake -D CMAKE_BUILD_TYPE=Debug .` instead

# Update repo properly (should be handled by most git GUI clients)
git submodule init
git submodule update --init --recursive
git submodule update --recursive

# For more info: http://github.com/cxong/cdogs-sdl/wiki/Developer-Getting-Started:-GCW-Zero
rm CMakeCache.txt
rm gcw0build -rf
mkdir gcw0build
cd gcw0build
cmake -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_DIR/gcw0-toolchain/usr/share/buildroot/toolchainfile.cmake" ..
make
