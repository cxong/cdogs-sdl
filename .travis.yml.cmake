language: c
dist: eoan
osx_image: xcode10.3

compiler:
  - gcc
  - clang
os:
  - linux
  - osx
env:
  - VERSION=@VERSION@

addons:
  apt:
    packages:
    - rpm
    - libsdl2-dev
    - libsdl2-image-dev
    - libsdl2-mixer-dev
    - clang-9
    - cmake
    - gcc-10
    - g++-10
    - libgtk-3-dev
    - ninja-build
  snaps:
  - protobuf

install:
# /usr/bin/gcc points to an older compiler on both Linux and macOS.
- if [ "$CXX" = "g++" ]; then export CXX="g++-10" CC="gcc-10"; fi
# /usr/bin/clang points to an older compiler on both Linux and macOS.
#
# Homebrew's llvm package doesn't ship a versioned clang++ binary, so the values
# below don't work on macOS. Fortunately, the path change above makes the
# default values (clang and clang++) resolve to the correct compiler on macOS.
- if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    if [ "$CXX" = "clang++" ]; then export CXX="clang++-9" CC="clang-9"; fi;
  fi
- echo ${CC}
- protoc --version

before_script:
  - export CTEST_OUTPUT_ON_FAILURE=1

script:
  - if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then sh build/macosx/install-sdl2.sh ; fi
  # Match install prefix with data dir so that package contains everything required
  - cmake -DCMAKE_INSTALL_PREFIX=. -DDATA_INSTALL_DIR=. -Wno-dev .
  - make -j2

  # Tests are broken on osx. Hope this will be fixed some day
  - if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then make test ; fi

after_success:
  - bash <(curl -s https://codecov.io/bash)

before_deploy:
  - make package

  #debug
  - ls $TRAVIS_BUILD_DIR

deploy:
  provider: releases
  edge: true
  token:
    secure: Rus8lTl0EnVqM6PXwleQ8cffjMTMY1gHGwVdbGsu8cWaDgAWQ86TFgGBbV+x12z9floDPzI7Z1K/entktkiSWQyRPIa9jQfJBIomNABhIykUvpRsL026Cs8TysI4L4hrTvFev10QI28RFyZvUDBT8yytowFsuU5Pfb4n7kDIisQ=
  file_glob: true
  file:
    - "$TRAVIS_BUILD_DIR/C-Dogs*SDL-*-{Linux,OSX}.{tar.gz,dmg}"
  on:
    tags: true
    condition: $CC = gcc

after_deploy:
  - bash build/travis-ci/butler.sh
