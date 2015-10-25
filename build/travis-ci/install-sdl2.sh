#!/bin/sh
set -e
export SDL_PREFIX=$HOME/sdl2
# check to see if sdl2 folder is empty
if [ ! -d "$SDL_PREFIX" ]; then
  curl -O http://www.libsdl.org/release/SDL2-2.0.3.tar.gz
  tar -xzvf SDL2-2.0.3.tar.gz
  (cd SDL2-2.0.3 && ./configure --prefix=$SDL_PREFIX && make && make install)
  curl -O https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.0.tar.gz
  tar -xzvf SDL2_image-2.0.0.tar.gz
  (cd SDL2_image-2.0.0 && ./configure --prefix=$SDL_PREFIX && make && make install)
  curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.0.tar.gz
  tar -xzvf SDL2_mixer-2.0.0.tar.gz
  (cd SDL2_mixer-2.0.0 && ./configure --prefix=$SDL_PREFIX && make && make install)
else
  echo "Using cached sdl2 directory."
fi
