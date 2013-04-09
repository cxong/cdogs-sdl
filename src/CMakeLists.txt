cmake_minimum_required(VERSION 2.8)
project(cdogs-sdl C)

SET(VERSION "0.4.2")

set(DEBUG 0)			#set to 1 to enable debug build
set(DEBUG_PROFILE 0)	# set to 1 to enable debug profile build

SET(DESTDIR /opt/cdogs-sdl/bin)
SET(LINKDIR /usr/games)

if(DEBUG)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)

if(APPLE)
	ADD_DEFINITIONS(-DSYS_MACOSX)
endif()
if(WINDOWS)
	ADD_DEFINITIONS(-DSYS_WIN)
endif()


execute_process(
	COMMAND "sdl-config" "--libs"
	OUTPUT_VARIABLE SDL_LDFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
	COMMAND "sdl-config" "--cflags"
	OUTPUT_VARIABLE SDL_INCLUDES
	OUTPUT_STRIP_TRAILING_WHITESPACE)

ADD_DEFINITIONS(${SDL_LDFLAGS} ${SDL_INCLUDES})
if(MSVC)
	ADD_DEFINITIONS(-W4 -WX)
	ADD_DEFINITIONS(-I${SDL_INCLUDE_DIR})
else()
	ADD_DEFINITIONS(-fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)
	if(DEBUG)
		ADD_DEFINITIONS(-g)
		if(DEBUG_PROFILE)
			ADD_DEFINITIONS(-p)
		endif(DEBUG_PROFILE)
	else()
		ADD_DEFINITIONS(-O2)
	endif()
endif()

include_directories(missions tinydir)

set(CDOGS_DATA_DIR "")
set(CDOGS_CFG_DIR ".cdogs/")
IF(MSVC)
	SET(SOURCE_DIRECTORY "${CMAKE_BINARY_DIR}")
	SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cdogs-sdl_${VERSION}")
ELSE()
	SET(SOURCE_DIRECTORY "..")
	SET(OUTPUT_DIRECTORY "../cdogs-sdl_${VERSION}")
ENDIF()
# TODO: fix workaround due to path difference
IF(MSVC)
	CONFIGURE_FILE(sys_config.h.cmake src/sys_config.h)
ELSE()
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "..")
	CONFIGURE_FILE(sys_config.h.cmake sys_config.h)
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.cmake ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl)
ENDIF()

SET(CDOGS_SOURCES
	actors.c
	ai.c
	automap.c
	blit.c
	defs.c
	draw.c
	drawtools.c
	events.c
	files.c
	game.c
	gamedata.c
	grafx.c
	hiscores.c
	input.c
	joystick.c
	keyboard.c
	map.c
	mission.c
	objs.c
	pics.c
	sounds.c
	text.c
	triggers.c
	utils.c)
SET(CDOGS_HEADERS
	actors.h
	ai.h
	automap.h
	blit.h
	defs.h
	draw.h
	drawtools.h
	events.h
	files.h
	game.h
	gamedata.h
	grafx.h
	hiscores.h
	input.h
	joystick.h
	keyboard.h
	map.h
	mission.h
	objs.h
	pics.h
	sounds.h
	sys_config.h
	sys_specifics.h
	text.h
	triggers.h
	utils.h)
add_library(cdogs STATIC ${CDOGS_SOURCES} ${CDOGS_HEADERS})

SET(CDOGS_SDL_SOURCES
	campaigns.c
	cdogs.c
	config.c
	credits.c
	drawtools.c
	mainmenu.c
	menu.c
	password.c
	prep.c)
SET(CDOGS_SDL_HEADERS
	campaigns.h
	config.h
	credits.h
	drawtools.h
	mainmenu.h
	menu.h
	password.h
	prep.h
	sys_config.h
	sys_specifics.h)
add_executable(cdogs-sdl ${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS})
target_link_libraries(cdogs-sdl cdogs ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
IF(UNIX AND NOT APPLE)	# only support installation in linux
	INSTALL(TARGETS cdogs-sdl DESTINATION ${DESTDIR})
	INSTALL(DIRECTORY
		${SOURCE_DIRECTORY}/doc
		${SOURCE_DIRECTORY}/missions
		${SOURCE_DIRECTORY}/dogfights
		${SOURCE_DIRECTORY}/graphics
		${SOURCE_DIRECTORY}/music
		${SOURCE_DIRECTORY}/sounds
		DESTINATION ${DESTDIR})
	#INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E create_symlink ${DESTDIR}/cdogs-sdl ${LINKDIR}/cdogs-sdl)")
	INSTALL(
		FILES ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl
		DESTINATION ${LINKDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
ENDIF()

SET(CDOGS_SDL_EDITOR_SOURCES
	cdogsed.c
	charsed.c)
SET(CDOGS_SDL_EDITOR_HEADERS
	charsed.h
	sys_config.h
	sys_specifics.h)
add_executable(cdogs-sdl-editor ${CDOGS_SDL_EDITOR_SOURCES} ${CDOGS_SDL_EDITOR_HEADERS})
target_link_libraries(cdogs-sdl-editor cdogs ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})



# Stage for packaging
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIRECTORY}")

# docs
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/doc"
		"${OUTPUT_DIRECTORY}/doc/")

# missions/dogfights
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/dogfights"
		"${OUTPUT_DIRECTORY}/dogfights/")
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/missions"
		"${OUTPUT_DIRECTORY}/missions/")

# data
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/graphics"
		"${OUTPUT_DIRECTORY}/graphics/")
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/music"
		"${OUTPUT_DIRECTORY}/music/")
ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SOURCE_DIRECTORY}/sounds"
		"${OUTPUT_DIRECTORY}/sounds/")

IF(MSVC)
	# DLLs
	FILE(GLOB DLLS "${SOURCE_DIRECTORY}/*.dll")
	FOREACH(DLL ${DLLS})
		ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
				"${DLL}"
				"${OUTPUT_DIRECTORY}/")
	ENDFOREACH()

	# exes
	SET(EXES
		Release/cdogs-sdl.exe
		#Release/cdogs-sdl-editor.exe
		)
	FOREACH(EXE ${EXES})
		ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
				"${EXE}"
				"${OUTPUT_DIRECTORY}/")
	ENDFOREACH()
ELSE()
	# binaries
	SET(BINS
		../cdogs-sdl
		#../cdogs-sdl-editor
		)
	FOREACH(BIN ${BINS})
		ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
				"${BIN}"
				"${OUTPUT_DIRECTORY}/")
	ENDFOREACH()

	# icon
	ADD_CUSTOM_COMMAND(TARGET cdogs-sdl POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			"${SOURCE_DIRECTORY}/cdogs_icon.bmp"
			"${OUTPUT_DIRECTORY}/")
ENDIF()
# TODO: package (with CPack?)
