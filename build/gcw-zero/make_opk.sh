#!/bin/sh

# usage: path/to/make_opk.sh [<binary directory> <data directory>]
# The parameters are needed, because cmake supports out-of-source builds.
# When missing, a in-source build is assumed and default directories are set.

mydir=`dirname $0`

if [ -z "$1" ] || [ -z "$2" ]; then
  bindir=${mydir}/../../src
  datadir=${mydir}/../..
else
  bindir=$1
  datadir=$2
fi

cdogsbin="${bindir}/cdogs-sdl"

cdogsdata="${datadir}/data"
cdogsdata="${cdogsdata} ${datadir}/doc"
cdogsdata="${cdogsdata} ${datadir}/dogfights"
cdogsdata="${cdogsdata} ${datadir}/graphics"
cdogsdata="${cdogsdata} ${datadir}/missions"
cdogsdata="${cdogsdata} ${datadir}/music"
cdogsdata="${cdogsdata} ${datadir}/sounds"

gcwzdata="${mydir}/default.gcw0.desktop"
gcwzdata="${gcwzdata} ${mydir}/readme_gcw.txt"
gcwzdata="${gcwzdata} ${mydir}/cdogs-sdl.png"

alldata="${gcwzdata} ${cdogsbin} ${cdogsdata}"

/opt/gcw0-toolchain/usr/bin/mksquashfs ${alldata} cdogs-sdl.opk -all-root -noappend -no-exports -no-xattrs
