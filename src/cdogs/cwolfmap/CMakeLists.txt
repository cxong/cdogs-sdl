set(CW_SOURCES audio.c audio_bs6.c audio_n3d.c audio_sod.c audio_wl1.c audio_wl6.c cwolfmap.c expand.c n3d.c vswap.c mame/fmopl.c)
set(CWHEADERS audiobs6.h audion3d.h AUDIOSOD.H AUDIOWL1.H audiowl6.h audio.h audio_bs6.h audio_n3d.h audio_sod.h audio_wl1.h audio_wl6.h common.h cwolfmap.h expand.h n3d.h vswap.h mame/fmopl.h)

add_subdirectory(wad)
add_subdirectory(zip)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
target_link_libraries(cwolfmap wad zip)
if(NOT WIN32)
	target_link_libraries(cwolfmap m)
endif()
