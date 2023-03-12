name: Build

on:
  push:
    branches: [ master ]
    tags:
    - '*'
  pull_request:
    branches: [ master ]
  release:
    types: [published, created, edited]

env:
  VERSION: @VERSION@
  CTEST_EXT_COLOR_OUTPUT: TRUE
  CTEST_OUTPUT_ON_FAILURE: 1
  CTEST_BUILD_FLAGS: -j4
  BUILD_TYPE: Release
  SDL_AUDIODRIVER: dummy
  SDL_VIDEODRIVER: dummy

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            cc: gcc
            cc_version: latest
          - os: ubuntu-latest
            cc: gcc
            cc_version: 11
          - os: ubuntu-latest
            cc: clang
            cc_version: latest
          - os: ubuntu-latest
            cc: clang
            cc_version: 12
          - os: macos-latest
          - os: windows-latest

    steps:
    - name: Checkout
      uses: actions/checkout@v3

    - name: Install Protoc
      uses: arduino/setup-protoc@v1.1.2
      with:
        version: '3.x'
        repo-token: ${{ secrets.GITHUB_TOKEN }}

    - name: Check protoc
      run: |
        protoc --version

    - name: Set up Homebrew (Linux)
      id: set-up-homebrew
      if: matrix.os == 'ubuntu-latest'
      uses: Homebrew/actions/setup-homebrew@master

    - name: Install SDL via homebrew (Linux)
      # Because ubuntu 22 doesn't have the latest SDL libs
      if: matrix.os == 'ubuntu-latest'
      run: brew install sdl2 sdl2_mixer sdl2_image

    - name: Install packages (Linux)
      if: startsWith(matrix.os, 'ubuntu')
      run: |
        sudo apt-get update
        sudo apt install python3-pip libgl1-mesa-dev
        python3 -m pip install protobuf
        pip3 install --upgrade protobuf
      # libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev

    - name: Set up GCC (Linux)
      if: startsWith(matrix.os, 'ubuntu') && matrix.cc == 'gcc'
      uses: egor-tensin/setup-gcc@v1
      with:
        version: ${{ matrix.cc_version }}

    - name: Set up Clang (Linux)
      if: startsWith(matrix.os, 'ubuntu') && matrix.cc == 'clang'
      uses: egor-tensin/setup-clang@v1
      with:
        version: ${{ matrix.cc_version }}

    - name: Install packages (macOS)
      if: matrix.os == 'macos-latest'
      run: |
        python3 -m pip install protobuf
        pip3 install --upgrade protobuf
        build/macosx/install-sdl2.sh

    - name: Install dependencies (Windows)
      if: matrix.os == 'windows-latest'
      run: C:\vcpkg\vcpkg.exe install --triplet x64-windows sdl2 sdl2-image sdl2-mixer[core,libvorbis,mpg123] protobuf --recurse

    - name: Configure CMake
      env:
        CC: ${{ matrix.cc }}
      if: matrix.os != 'windows-latest'
      # Configure CMake in a 'build' subdirectory. `CMAKE_BUILD_TYPE` is only required if you are using a single-configuration generator such as make.
      # See https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html?highlight=cmake_build_type
      run: cmake -B . -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} -DCMAKE_INSTALL_PREFIX=. -DDATA_INSTALL_DIR=. -Wno-dev

    - name: Configure CMake (Windows)
      if: matrix.os == 'windows-latest'
      run: cmake -B . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

    - name: Build
      # Build your program with the given configuration
      run: cmake --build .

    - name: Test
      working-directory: ${{github.workspace}}
      # Execute tests defined by the CMake configuration.  
      # See https://cmake.org/cmake/help/latest/manual/ctest.1.html for more detail
      run: ctest -C Release -VV -S

    - name: Make package on tags
      if: startsWith(github.ref, 'refs/tags/')
      run: |
        make package

    - name: Upload a Build Artifact
      uses: softprops/action-gh-release@v1
      if: startsWith(github.ref, 'refs/tags/')
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
      with:
        files: ${{ github.workspace }}/C-Dogs*SDL-*-*.*
        fail_on_unmatched_files: true

    - name: Publish to itch.io (Linux)
      if: startsWith(github.ref, 'refs/tags/') && startsWith(matrix.os, 'ubuntu') && matrix.cc == 'gcc' && matrix.cc_version == 'latest' && !github.event.release.prerelease
      env:
        BUTLER_API_KEY: ${{ secrets.BUTLER_API_KEY }}
      run: |
        curl -L -o butler.zip https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default
        unzip butler.zip
        chmod +x butler
        ./butler -V
        ./butler push C-Dogs*SDL-*-Linux.tar.gz congusbongus/cdogs-sdl:linux --userversion $VERSION

    - name: Publish to itch.io (macOS)
      if: startsWith(github.ref, 'refs/tags/') && matrix.os == 'macos-latest' && !github.event.release.prerelease
      env:
        BUTLER_API_KEY: ${{ secrets.BUTLER_API_KEY }}
      run: |
        curl -L -o butler.zip https://broth.itch.ovh/butler/darwin-amd64/LATEST/archive/default
        unzip butler.zip
        chmod +x butler
        ./butler -V
        ./butler push C-Dogs*SDL-*-OSX.dmg congusbongus/cdogs-sdl:mac --userversion $VERSION
