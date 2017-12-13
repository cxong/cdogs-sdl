#!/bin/bash

rm -rf emscripten/*
mkdir -p emscripten

cmake .

emcc -D "PB_FIELD_16BIT=1" \
    -Isrc/ \
    -Isrc/cdogs/ \
    -Isrc/cdogs/proto/nanopb/ \
    -Isrc/cdogs/enet/include/ \
    -Isrc/cdogs/include/ \
    -Isrc/tests/ \
    src/*.c \
    $(find src/cdogs/ -name "*.c") \
    src/json/*.c \
    -g4 \
    -O0 \
    -s ASSERTIONS=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s USE_SDL=2 \
    -s USE_SDL_IMAGE=2 \
    -s SDL2_IMAGE_FORMATS='["png"]' \
    -s USE_VORBIS=1 \
    -s USE_OGG=1 \
    --preload-file data \
    --preload-file doc \
    --preload-file dogfights \
    --preload-file graphics \
    --preload-file missions \
    --preload-file music \
    --preload-file sounds \
    -o emscripten/index.html
