cmake_minimum_required(VERSION 2.8)
add_definitions(-DSTATIC)
set(HQX_SOURCES
	common.c
	hq2x.c
	hq3x.c
	hq4x.c
	init.c)
set(HQX_HEADERS
	common.h
	hqx.h)
add_library(hqx STATIC ${HQX_SOURCES} ${HQX_HEADERS})
set_target_properties(hqx PROPERTIES PREFIX "lib")
if(MSVC)
	set_target_properties(hqx PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
	set_target_properties(hqx PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ..)
	set_target_properties(hqx PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ..)
endif(MSVC)
