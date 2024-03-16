set(ZIP_SOURCES zip.c)
set(ZIP_HEADERS zip.h miniz.h)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP -W4 -WX -wd\"4127\" -wd\"4244\" -wd\"4267\" -wd\"4706\"")
else()
	add_definitions(
		-fsigned-char
		-Wall -W
		-Wstrict-prototypes -Wpointer-arith
		-Wno-cast-qual -Wno-conversion)
endif()

add_library(zip STATIC ${ZIP_SOURCES} ${ZIP_HEADERS})
