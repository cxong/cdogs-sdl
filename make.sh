#!/bin/sh
if [ "$(uname -s | cut -c0-5)" == "MINGW" ] ; then
	cmake -G"MinGW Makefiles" src
else
	cmake src
fi
cd src
if [ "$(uname -s | cut -c0-5)" == "MINGW" ] ; then
	mingw32-make
else
	make
fi
cd ..

