version: @VERSION@.{build}

branches:
  except:
    - gh-pages

clone_folder: c:\projects\cdogs-sdl
image:
- Visual Studio 2019
configuration:
- Release
matrix:
  fast_finish: true
environment:
  CTEST_OUTPUT_ON_FAILURE: 1
  PYTHON: "C:\\Python39-x64"
  SDL2_VERSION: 2.0.10
  SDL2_IMAGE_VERSION: 2.0.5
  SDL2_MIXER_VERSION: 2.0.4
  SDLDIR: C:\projects\cdogs-sdl
  VERSION: @VERSION@

install:
  - "SET PATH=%PYTHON%;%PYTHON%\\Scripts;%PATH%"
  - cd C:\Tools\vcpkg
  - git pull
  - .\bootstrap-vcpkg.bat
  - cd %APPVEYOR_BUILD_FOLDER%
  - vcpkg install sdl2 sdl2-image sdl2-mixer protobuf --recurse
  - pip install protobuf

before_build:
  - .\build\windows\get-sdl2-dlls.bat dll "appveyor DownloadFile"
  - cmake -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 16 2019" -A Win32 .

build:
  project: c:\projects\cdogs-sdl\cdogs-sdl.sln
  verbosity: minimal
  parallel: true

after_build:
  - msbuild c:\projects\cdogs-sdl\src\PACKAGE.vcxproj
  - dir

cache:
- c:\tools\vcpkg\installed\

artifacts:
  - path: /.\C-Dogs*.exe/
  - path: /.\C-Dogs*.zip/

deploy:
  provider: GitHub
  description: ''
  auth_token:
    secure: Ap9XXGG/1cyV9hpat7EaYxZHBt/VWdH82Bx+nIugdJ1Dh3I9e8OP4L/IXkadUjdR
  prerelease: false
  force_update: true 	#to be in piece with Travis CI
  on:
    appveyor_repo_tag: true

after_deploy:
  - .\build\appveyor\butler.bat
