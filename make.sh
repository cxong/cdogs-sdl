#!/bin/sh

# To create a debug build, run `cmake -D CMAKE_BUILD_TYPE=Debug .` instead

if command -v ninja > /dev/null 2>&1; then
	echo "Build using Ninja"
	cmake -GNinja .
	ninja
else
	echo "Build using default makefiles"
	cmake .
	make
fi
