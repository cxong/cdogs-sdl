#!/bin/sh
cd src
UNAME_SHORT=`uname -s | cut -c1-5`
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	cmake -G"MinGW Makefiles" .
else
	cmake .
fi
if [ "$UNAME_SHORT" = "MINGW" ] ; then
	mingw32-make
else
	make
fi
cd ..
