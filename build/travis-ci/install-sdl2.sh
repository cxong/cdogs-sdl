#!/bin/sh
set -e
curl -O http://www.libsdl.org/release/SDL2-2.0.4.tar.gz
tar -xzvf SDL2-2.0.4.tar.gz
(cd SDL2-2.0.4 && ./configure && make && sudo make install)
curl -O https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.1.tar.gz
tar -xzvf SDL2_image-2.0.1.tar.gz
(cd SDL2_image-2.0.1 && ./configure && make && sudo make install)
curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.1.tar.gz
tar -xzvf SDL2_mixer-2.0.1.tar.gz
(cd SDL2_mixer-2.0.1 && ./configure && make && sudo make install)
