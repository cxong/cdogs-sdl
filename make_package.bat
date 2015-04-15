cmake -G "MinGW Makefiles" -D CMAKE_C_COMPILER=mingw32-gcc.exe -D CMAKE_MAKE_PROGRAM=mingw32-make.exe .
mingw32-make
mingw32-make package
pause