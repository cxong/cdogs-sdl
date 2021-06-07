set(CW_SOURCES audio.c cwolfmap.c expand.c vswap.c mame/fmopl.c)
set(CWHEADERS AUDIOWL1.H audiowl6.h audio.h cwolfmap.h expand.h vswap.h mame/fmopl.h)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
if(NOT WIN32)
	target_link_libraries(cwolfmap m)
endif()
