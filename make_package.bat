cmake -G "MinGW Makefiles" -D CMAKE_C_COMPILER=mingw32-gcc.exe -D CMAKE_MAKE_PROGRAM=mingw32-make.exe .
build\windows\get-sdl2-dlls.bat dll
mingw32-make
mingw32-make package
pause
