if(MSVC)
	add_definitions(-wd"4996")
else()
	add_definitions(-Wno-error)
endif()

add_library(json STATIC json.c json.h)
