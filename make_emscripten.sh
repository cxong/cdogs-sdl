#!/bin/bash
set -e

rm -rf emscripten/*
mkdir -p emscripten

emcc -D "PB_FIELD_16BIT=1" \
    -Isrc/ \
    -Isrc/cdogs/ \
    -Isrc/proto/nanopb/ \
    -Isrc/cdogs/enet/include/ \
    -Isrc/cdogs/include/ \
    -Isrc/tests/ \
    src/*.c \
    $(find src/cdogs/ -name "*.c") \
    src/json/*.c \
    src/proto/*.c \
    src/proto/nanopb/*.c \
    -c \
    -O3 \
    -s USE_SDL=2 \
    -s USE_SDL_IMAGE=2 \
    -s USE_SDL_MIXER=2 \
    -s SDL2_IMAGE_FORMATS='["png"]' \
    -s USE_VORBIS=1 \
    -s USE_OGG=1

emcc -O3 \
     *.o \
     -o emscripten/index.html \
     -s USE_SDL=2 \
     -s USE_SDL_IMAGE=2 \
     -s SDL2_IMAGE_FORMATS='["png"]' \
     -s USE_SDL_MIXER=2 \
     -s SDL2_MIXER_FORMATS='["ogg"]' \
     -lidbfs.js \
     -sASYNCIFY \
     -sSTACK_SIZE=131072 \
     -sINITIAL_MEMORY=128mb \
     --preload-file data \
     --preload-file doc \
     --preload-file dogfights \
     --preload-file graphics \
     --preload-file missions \
     --preload-file music \
     --preload-file sounds

cp build/windows/cdogs-icon.ico emscripten/favicon.ico
