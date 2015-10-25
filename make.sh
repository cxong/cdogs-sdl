#!/bin/sh

# To create a debug build, run `cmake -D CMAKE_BUILD_TYPE=Debug .` instead

UNAME_SHORT=`uname -s | cut -c1-5`
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	cmake -G"MinGW Makefiles" .
else
	cmake .
fi
make
