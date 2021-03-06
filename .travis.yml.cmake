language: c
dist: focal

env:
  global:
    - VERSION=@VERSION@
    - CTEST_EXT_COLOR_OUTPUT=TRUE
    - CTEST_BUILD_FLAGS=-j4
    - SDL_AUDIODRIVER=dummy
    - SDL_VIDEODRIVER=dummy

matrix:
  include:
    - os: linux
      osx_image: xcode12.2
      compiler: gcc
      env: CTEST_TARGET_SYSTEM=Linux-gcc    CTEST_MODEL=Nightly
      addons:
        apt:
          sources:
            - ppa:ubuntu-toolchain-r/test
          packages:
            # disable for now
            # - valgrind
            - libsdl2-dev
            - libsdl2-image-dev
            - libsdl2-mixer-dev
            - gcc-10
            - g++-10
            - libgtk-3-dev
            - python3-pip
    - os: osx
      compiler: clang
      env: CTEST_TARGET_SYSTEM=MacOS-clang  CTEST_MODEL=Nightly
      addons:
        apt:
          packages:
            - clang-9

before_install:
- if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then wget https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protoc-3.12.3-linux-x86_64.zip; fi
- if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then unzip protoc-3.12.3-linux-x86_64.zip; fi
- if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then sudo mv bin/protoc /usr/bin; fi
- if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then sudo mv include/* /usr/local/include; fi
- python3 -m pip install protobuf

install:
# /usr/bin/gcc points to an older compiler on both Linux and macOS.
- if [ "$CXX" = "g++" ]; then export CXX="g++-10" CC="gcc-10"; fi
- echo ${CC}
- protoc --version

before_script:
  - export CTEST_OUTPUT_ON_FAILURE=1

script:
  - if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then sh build/macosx/install-sdl2.sh ; fi
  # Match install prefix with data dir so that package contains everything required
  - cmake -DCMAKE_INSTALL_PREFIX=. -DDATA_INSTALL_DIR=. -Wno-dev .
  - make
  - ctest -VV -S
  # Disable valgrind for now; memory errors to be fixed
  # - if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then cd src && valgrind ./cdogs-sdl --demo; fi

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

after_deploy:
  - bash build/travis-ci/butler.sh
