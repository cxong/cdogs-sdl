Remove-Item .\emscripten -Force -Recurse
New-Item -ItemType Directory emscripten

cmake -G "MinGW Makefiles" -D CMAKE_C_COMPILER=mingw32-gcc.exe -D CMAKE_MAKE_PROGRAM=mingw32-make.exe -D CMAKE_BUILD_TYPE=Debug .

emcc -D "PB_FIELD_16BIT=1" `
    -Isrc/ `
    -Isrc/cdogs/ `
    -Isrc/cdogs/proto/nanopb/ `
    -Isrc/cdogs/enet/include/ `
    -Isrc/cdogs/include/ `
    -Isrc/tests/ `
    @(Get-ChildItem -Path src/*.c) `
    @(Get-ChildItem -Path .\src\cdogs\*.c -Recurse -File) `
    @(Get-ChildItem -Path src/json/*.c) `
    -O0 -g4 `
    -s ASSERTIONS=1 `
    -s ALLOW_MEMORY_GROWTH=1 `
    -s USE_SDL=2 `
    -s USE_SDL_IMAGE=2 `
    -s USE_SDL_MIXER=2 `
    -s USE_OGG=1 `
    -s USE_VORBIS=1 `
    --preload-file data `
    --preload-file doc `
    --preload-file dogfights `
    --preload-file graphics `
    --preload-file missions `
    --preload-file music `
    --preload-file sounds `
    -o emscripten/index.html `
    -s @'
SDL2_IMAGE_FORMATS="[""png""]"
'@

Copy-Item .\build\windows\cdogs-icon.ico emscripten/favicon.ico
