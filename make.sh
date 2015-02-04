#!/bin/sh

# To create a debug build, run `cmake -D CMAKE_BUILD_TYPE=Debug .` instead

# Update repo properly (should be handled by most git GUI clients)
git submodule init
git submodule update --init --recursive
git submodule update --recursive

UNAME_SHORT=`uname -s | cut -c1-5`
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	cmake -G"MinGW Makefiles" .
else
	cmake .
fi
make
