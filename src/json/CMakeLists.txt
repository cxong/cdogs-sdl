if(MSVC)
	add_definitions(-wd"4996")
endif()

add_library(json STATIC json.c json.h)