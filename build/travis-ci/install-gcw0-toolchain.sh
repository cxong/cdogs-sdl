#!/bin/sh
set -e
# check to see if toolchain folder is empty
if [ ! -d "/opt/gcw0-toolchain" ]; then
  if [ ! -f "opendingux-gcw0-toolchain.2014-08-20.tar.bz2" ]; then
    curl -O http://www.gcw-zero.com/files/opendingux-gcw0-toolchain.2014-08-20.tar.bz2
  fi
  (sudo mkdir -p /opt && sudo tar jxf opendingux-gcw0-toolchain.2014-08-20.tar.bz2 -C /opt)
else
  echo "Using cached toolchain directory."
fi
