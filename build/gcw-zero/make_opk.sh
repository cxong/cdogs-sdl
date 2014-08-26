#/bin/bash

# usage: path/to/make_opk.sh [<binary directory> <data directory>]
# The parameters are needed, because cmake supports out-of-source builds.
# When missing, a in-source build is assumed and default directories are set.

mydir=`dirname $0`

if [[ -z "$1" || -z "$2" ]]; then
  bindir=${mydir}/../../src
  datadir=${mydir}/../..
else
  bindir=$1
  datadir=$2
fi

cdogsbin="${bindir}/cdogs-sdl"

cdogsdata=("${datadir}/data"
           "${datadir}/doc"
           "${datadir}/dogfights"
           "${datadir}/graphics"
           "${datadir}/missions"
           "${datadir}/music"
           "${datadir}/sounds"
           "${datadir}/cdogs_icon.bmp")

gcwzdata=("${mydir}/default.gcw0.desktop"
          "${mydir}/readme_gcw.txt"
          "${mydir}/cdogs-sdl.png")

alldata="${gcwzdata[*]} ${cdogsbin} ${cdogsdata[*]}"

mksquashfs ${alldata} cdogs.opk -all-root -noappend -no-exports -no-xattrs
