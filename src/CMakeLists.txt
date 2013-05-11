cmake_minimum_required(VERSION 2.8)
project(cdogs-sdl C)

SET(VERSION "0.4.2.1")
SET(VERSION_RC "0,4,2,1")
SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "2")
SET(WEBSITE "http://cxong.github.io/cdogs-sdl/")

set(DEBUG 0)			#set to 1 to enable debug build
set(DEBUG_PROFILE 0)	# set to 1 to enable debug profile build

IF(UNIX AND NOT APPLE)
	SET(CMAKE_INSTALL_PREFIX /opt/cdogs-sdl/bin)
ENDIF()
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
	COMMAND "sh" "sdl-config" "--libs"
	OUTPUT_VARIABLE SDL_LDFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
	COMMAND "sh" "sdl-config" "--cflags"
	OUTPUT_VARIABLE SDL_CFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

ADD_DEFINITIONS(${SDL_LDFLAGS} ${SDL_CFLAGS})
ADD_DEFINITIONS(-I${SDL_INCLUDE_DIR})
if(MSVC)
	ADD_DEFINITIONS(-W4 -WX)
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
IF(UNIX)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "..")
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.cmake ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl)
ENDIF()
IF(MSVC)
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/windows/cdogs.rc.cmake ${SOURCE_DIRECTORY}/build/windows/cdogs.rc)
ENDIF()
CONFIGURE_FILE(sys_config.h.cmake ${SOURCE_DIRECTORY}/src/sys_config.h)
CONFIGURE_FILE(${SOURCE_DIRECTORY}/README.md.cmake ${SOURCE_DIRECTORY}/README.md)

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
SET(CDOGS_SDL_EXTRA)
IF(MSVC)
	SET(CDOGS_SDL_EXTRA ${SOURCE_DIRECTORY}/build/windows/cdogs.rc)
ENDIF()
add_executable(cdogs-sdl ${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS} ${CDOGS_SDL_EXTRA})
target_link_libraries(cdogs-sdl cdogs ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})

# Installation
INSTALL(TARGETS cdogs-sdl DESTINATION .)
INSTALL(DIRECTORY
	${SOURCE_DIRECTORY}/doc
	${SOURCE_DIRECTORY}/missions
	${SOURCE_DIRECTORY}/dogfights
	${SOURCE_DIRECTORY}/graphics
	${SOURCE_DIRECTORY}/music
	${SOURCE_DIRECTORY}/sounds
	DESTINATION .)
INSTALL(FILES ${SOURCE_DIRECTORY}/cdogs_icon.bmp ${SOURCE_DIRECTORY}/README.md DESTINATION .)
IF(UNIX AND NOT APPLE)
	INSTALL(
		FILES ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl
		DESTINATION ${LINKDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	INSTALL(FILES
		${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.desktop
		${SOURCE_DIRECTORY}/cdogs_icon.png
		DESTINATION .)
ENDIF()
IF(MSVC)
	# Package for Windows
	FILE(GLOB DLLS "${SOURCE_DIRECTORY}/*.dll")
	FOREACH(DLL ${DLLS})
		INSTALL(FILES "${DLL}" DESTINATION .)
	ENDFOREACH()
ENDIF()

# Packaging
SET(CPACK_PACKAGE_NAME "C-Dogs SDL")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C-Dogs SDL: Action/Arcade Game")
SET(CPACK_PACKAGE_VERSION ${VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_PACKAGE_EXECUTABLES "..\\\\cdogs-sdl;C-Dogs SDL")
IF(MSVC)
	SET(CPACK_GENERATOR NSIS)
	SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	SET(CPACK_NSIS_MUI_ICON ${SOURCE_DIRECTORY}/cdogs-icon.ico)
	SET(CPACK_CREATE_DESKTOP_LINKS cdogs-sdl)
	SET(CPACK_RESOURCE_FILE_LICENSE ${SOURCE_DIRECTORY}/doc/COPYING.GPL)
	SET(CPACK_NSIS_MUI_FINISHPAGE_RUN ..\\\\cdogs-sdl.exe)
	SET(CPACK_NSIS_URL_INFO_ABOUT ${WEBSITE})
ELSE()
	SET(CPACK_GENERATOR TGZ)
ENDIF()
INCLUDE(CPack)

SET(CDOGS_SDL_EDITOR_SOURCES
	cdogsed.c
	charsed.c)
SET(CDOGS_SDL_EDITOR_HEADERS
	charsed.h
	sys_config.h
	sys_specifics.h)
add_executable(cdogs-sdl-editor ${CDOGS_SDL_EDITOR_SOURCES} ${CDOGS_SDL_EDITOR_HEADERS})
target_link_libraries(cdogs-sdl-editor cdogs ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
