#!/bin/bash

# mingw32-buildpkg.sh - build windows zip
#
# should be run from /tools

TREE=${TREE:-../..}
TRUNK=${TRUNK:-$TREE/trunk}

VER=${VER:-unknown}
PKG=${PKGFILE:-cdogs-$VER-win32.zip}

MINGW_BIN=${MINGW_BIN:-/usr/mingw32/usr/bin}

PKG_DIR=/tmp/cdogs-$VER-$RANDOM

ZIP_NOTE="This is an experimental release. Windows XP or newer is suggested."

function die () {
	local msg=$*
	
	echo $msg
	exit 1
}

old=`pwd`

cd $TREE
TREE=`pwd`

cd $old

cd $TRUNK
TRUNK=`pwd`


export PATH="${MINGW_BIN}:$PATH"

echo "Trunk: $TRUNK"
echo "Pkg:   $PKG"

mkdir $PKG_DIR || die "Couldn't mkdir $PKG_DIR"

echo "Pdir:  $PKG_DIR"

cd src || die "Couldn't chdir into /src"

echo
echo "Building..."

if [ -f cdogs ] ; then
	make clean
fi

make \
	SYSTEM=\"mingw32\" \
	SOUND_CODE=\"sdlmixer\" \
	PREFIX=\".\" \
	BINDIR=\"./\" \
	DATADIR=\"./data\" \
	CFGDIR=\"./config\" \
	I_AM_CONFIGURED=yes \
	cdogs || die "Build failed."

[ ! -f cdogs.exe ] && die "Build failed? Couldn't find cdogs.exe"

echo
echo "Executables..."

cp cdogs.exe $PKG_DIR

echo
echo "DLL's..."

cp $MINGW_BIN/SDL.dll $PKG_DIR
cp $MINGW_BIN/SDL_mixer.dll $PKG_DIR

old=`pwd`

echo
echo "Docs..."

svn export $TRUNK/doc $PKG_DIR/doc || die "Couldn't export data to $PKG_DIR/doc"

echo
echo "Data..."

svn export $TREE/branches/cdogs-data $PKG_DIR/data || die "Couldn't export data to $PKG_DIR/data"

echo
echo "Packaging..."

cd $PKG_DIR

zip -r -9 $PKG * && echo "Done! See $PKG in $PKG_DIR."

echo "$ZIP_NOTE" | zipnote -w $PKG
