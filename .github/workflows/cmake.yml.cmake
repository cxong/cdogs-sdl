name: Build

on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

env:
  VERSION: @VERSION@
  CTEST_EXT_COLOR_OUTPUT: TRUE
  CTEST_OUTPUT_ON_FAILURE: 1
  CTEST_BUILD_FLAGS: -j4
  SDL_AUDIODRIVER: dummy
  SDL_VIDEODRIVER: dummy

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ ubuntu-latest, macos-latest ]

    steps:
    - uses: actions/checkout@v2

    - name: Install Protoc
      uses: arduino/setup-protoc@v1.1.2
      with:
        version: '3.12.3'
        repo-token: ${{ secrets.GITHUB_TOKEN }}

    - name: Check protoc
      run: |
        protoc --version

    - name: Install packages Linux
      if: matrix.os == 'ubuntu-latest'
      run: |
        sudo apt-get update
        sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev gcc-10 g++-10 libgtk-3-dev python3-pip
        python3 -m pip install protobuf
        pip3 install --upgrade protobuf

    - name: Install packages macOS
      if: matrix.os == 'macos-latest'
      run: |
        python3 -m pip install protobuf
        pip3 install --upgrade protobuf
        build/macosx/install-sdl2.sh

    - name: Configure CMake
      # Configure CMake in a 'build' subdirectory. `CMAKE_BUILD_TYPE` is only required if you are using a single-configuration generator such as make.
      # See https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html?highlight=cmake_build_type
      run: cmake -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} -DCMAKE_INSTALL_PREFIX=. -DDATA_INSTALL_DIR=. -Wno-dev .

    - name: Build
      # Build your program with the given configuration
      run: make

    - name: Test
      working-directory: ${{github.workspace}}
      # Execute tests defined by the CMake configuration.  
      # See https://cmake.org/cmake/help/latest/manual/ctest.1.html for more detail
      run: ctest -VV -S

    - name: Deploy on tags
      if: startsWith(github.ref, 'refs/tags/')
      run: |
        make package
        bash build/travis-ci/butler.sh

    - name: Upload a Build Artifact
      uses: actions/upload-artifact@v2.2.4
      if: startsWith(github.ref, 'refs/tags/')
      with:
        path: C-Dogs*SDL-*-*.*

