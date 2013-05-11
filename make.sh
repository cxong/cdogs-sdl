#!/bin/sh
cmake src
cd src
if [ "$(uname -s | cut -c0-5)" == "MINGW" ] ; then
	mingw32-make
else
	make
fi
cd ..

