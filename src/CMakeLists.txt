cmake_minimum_required(VERSION 2.6)
project(cdogs-sdl)

#There are lots of scripts with cmake
#for finding external libraries. 
#see /usr/local/share/cmake-2.6/Modules/Find*.cmake for more examples
find_package(sdl-dev REQUIRED)
find_package(sdl-mixer)

set(CF_OPTIMISE "-02")
set(CF_ARCH "")
set(CF_DEBUG "-w") #-ggdb -Wall, -pg
set(CMAKE_C_FLAGS "-g -Wall")
add_subdirectory(w01-cpp)
