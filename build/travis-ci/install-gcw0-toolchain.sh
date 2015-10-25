#!/bin/sh
set -e
export TOOLCHAIN=$HOME/gcw0-toolchain
# check to see if toolchain folder is empty
if [ ! -d "$TOOLCHAIN" ]; then
  curl -O http://www.gcw-zero.com/files/opendingux-gcw0-toolchain.2014-08-20.tar.bz2
  tar jxvf opendingux-gcw0-toolchain.2014-08-20.tar.bz2
else
  echo "Using cached toolchain directory."
fi
