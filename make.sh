#!/bin/sh
UNAME_SHORT=`uname -s | cut -c1-5`
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	cmake -G"MinGW Makefiles" src
	cd src
else
	cd src
	cmake .
fi
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	mingw32-make
else
	make
fi
cd ..
