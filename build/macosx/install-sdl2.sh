#!/bin/sh
set -e

OSX_LIB_PATH="/Library/Frameworks"

OSX_SDL2_PATH_FULL="$OSX_LIB_PATH/SDL2.framework"
OSX_SDL2_IMAGE_FULLPATH="$OSX_LIB_PATH/SDL2_image.framework"
OSX_SDL2_MIXER_FULLPATH="$OSX_LIB_PATH/SDL2_mixer.framework"

function getSdl2 {
  echo "Starting downloading sdl2..."
  if [ ! -f "SDL2-2.0.5.dmg" ]; then
    curl -O https://www.libsdl.org/release/SDL2-2.0.5.dmg
  fi
  mkdir -p $OSX_LIB_PATH
  7z x -y SDL2-2.0.5.dmg
  cd SDL2
  chmod -R 0755 SDL2.framework
  sudo cp -rv SDL2.framework $OSX_LIB_PATH/
  cd ..
}

function getSdl2_image {
  echo "Starting downloading sdl2_image..."
  if [ ! -f "SDL2_image-2.0.1.dmg" ]; then
    curl -O https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.1.dmg
  fi
  mkdir -p $OSX_LIB_PATH
  7z x -y SDL2_image-2.0.1.dmg
  cd SDL2_image
  chmod -R 0755 SDL2_image.framework
  sudo cp -r SDL2_image.framework $OSX_LIB_PATH/
  cd ..
}

function getSdl2_mixer {
  echo "Starting downloading sdl2_mixer..."
  if [ ! -f "SDL2_mixer-2.0.1.dmg" ]; then
    curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.1.dmg
  fi
  mkdir -p $OSX_LIB_PATH
  7z x -y SDL2_mixer-2.0.1.dmg
  cd SDL2_mixer
  chmod -R 0755 SDL2_mixer.framework
  sudo cp -r SDL2_mixer.framework $OSX_LIB_PATH/
  cd ..
}


if [[ "$1" == "--force" ]]
then
echo "Force build requested by --force."
getSdl2
getSdl2_image
getSdl2_mixer
exit
fi

# check to see if folder is empty
if [[ -d "$OSX_SDL2_PATH_FULL"  ]]; then
  if [[ -n "$(ls -A $OSX_SDL2_PATH_FULL)" ]]
  then
    echo "SDL2 exists. Skip building..."
  else
    echo "SDL2 dir exists buit it is empty..."
    getSdl2
  fi
else
   echo "SDL2 dir missing..."
   getSdl2
fi

if [[ -d "$OSX_SDL2_IMAGE_PATH_FULL"  ]]; then
  if [[ -n "$(ls -A $OSX_SDL2_IMAGE_PATH_FULL)" ]]
  then
    echo "SDL2_image exists. Skip building..."
  else
    echo "SDL2_image dir exists buit it is empty..."
    getSdl2_image
  fi
else
   echo "SDL2_image dir missing..."
   getSdl2_image
fi

if [[ -d "$OSX_SDL2_MIXER_PATH_FULL"  ]]; then
  if [[ -n "$(ls -A $OSX_SDL2_MIXER_PATH_FULL)" ]]
  then
    echo "SDL2_mixer exists. Skip building..."
  else
    echo "SDL2_mixer dir exists buit it is empty..."
    getSdl2_mixer
  fi
else
   echo "SDL2_mixer dir missing..."
   getSdl2_mixer
fi
