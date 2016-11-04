#!/bin/sh
set -e

OSX_SDL2_PATH_FULL="/Library/Frameworks/SDL2.framework"

function getSdl2 {
  echo "Starting downloading sdl2..."
  if [ ! -f "SDL2-2.0.5.dmg" ]; then
    curl -O https://www.libsdl.org/release/SDL2-2.0.5.dmg
  fi
  mkdir -p /Library/Frameworks
  7z x -y SDL2-2.0.5.dmg
  cd SDL2
  sudo cp -r SDL2.framework/ /Library/Frameworks/
}

if [[ "$1" == "--force" ]]
then
echo "Force build requested by --force."
getSdl2
exit
fi

# check to see if toolchain folder is empty
if [[ -d "$OSX_SDL2_PATH_FULL"  ]]; then
  if [[ -n "$(ls -A $OSX_SDL2_PATH_FULL)" ]]
  then
    echo "Toolchain exists. Skip building..."
    exit
  else
  	echo "Toolchain dir exists buit it is empty..."
  	getSdl2
  	exit
  fi
else
	echo "Toolchain dir missing..."
	getSdl2
	exit
fi
