set(CW_SOURCES audio.c audio_sod.c audio_wl1.c audio_wl6.c cwolfmap.c expand.c vswap.c mame/fmopl.c)
set(CWHEADERS AUDIOSOD.H AUDIOWL1.H audiowl6.h audio.h audio_sod.h audio_wl1.h audio_wl6.h common.h cwolfmap.h expand.h vswap.h mame/fmopl.h)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
if(NOT WIN32)
	target_link_libraries(cwolfmap m)
endif()
