#!/bin/sh
set -e

GCW0_TOOLCHAIN_PATH="/opt/gcw0-toolchain"

function getToolchain {
  echo "Starting building GCW-Zero..."
  if [ ! -f "opendingux-gcw0-toolchain.2014-08-20.tar.bz2" ]; then
    curl -O http://www.gcw-zero.com/files/opendingux-gcw0-toolchain.2014-08-20.tar.bz2
  fi
  (sudo mkdir -p /opt && sudo tar jxf opendingux-gcw0-toolchain.2014-08-20.tar.bz2 -C /opt)
}

if [[ "$1" == "--force" ]]
then
echo "Force build requested by --force."
getToolchain
exit
fi

# check to see if toolchain folder is empty
if [[ -d "$GCW0_TOOLCHAIN_PATH"  ]]; then
  if [[ -n "$(ls -A $GCW0_TOOLCHAIN_PATH)" ]]
  then
    echo "Toolchain exists. Skip building..."
    exit
  else
  	echo "Toolchain dir exists buit it is empty..."
  	getToolchain
  	exit
  fi
else
	echo "Toolchain dir missing..."
	getToolchain
	exit
fi
