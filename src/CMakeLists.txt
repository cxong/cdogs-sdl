cmake_minimum_required(VERSION 2.8)
project(cdogs-sdl C)

SET(VERSION "0.4.4")
SET(VERSION_RC "0,4,4,0")	# this must be 4 numbers
SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "4")
SET(WEBSITE "http://cxong.github.io/cdogs-sdl/")

set(DEBUG 0)			#set to 1 to enable debug build
set(DEBUG_PROFILE 0)	# set to 1 to enable debug profile build

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG 1)
endif()

IF(UNIX AND NOT APPLE)
	SET(CMAKE_INSTALL_PREFIX /opt/cdogs-sdl/bin)
ENDIF()
if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	SET(CMAKE_RC_COMPILE_OBJECT
		"<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()
SET(LINKDIR /usr/games)

if(DEBUG)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)


execute_process(
	COMMAND "sdl-config" "--libs"
	OUTPUT_VARIABLE SDL_LDFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
	COMMAND "sdl-config" "--cflags"
	OUTPUT_VARIABLE SDL_CFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
# Work-around for CMake SDL bug (need to link SDLmain before SDL)
set(SDLMAIN_INDEX 0)
set(SDL_INDEX 0)
set(SDLMAIN_LIBRARY "")
set(INDEX 0)
foreach(SDL_LIBRARY_ITEM ${SDL_LIBRARY})
	string(FIND ${SDL_LIBRARY_ITEM} "libSDLmain" SDLMAIN_FIND_INDEX)
	string(FIND ${SDL_LIBRARY_ITEM} "libSDL.dll" SDL_FIND_INDEX)
	if(${SDLMAIN_FIND_INDEX} GREATER -1)
		SET(SDLMAIN_INDEX ${INDEX})
		set(SDLMAIN_LIBRARY ${SDL_LIBRARY_ITEM})
	endif()
	if(${SDL_FIND_INDEX} GREATER -1)
		SET(SDL_INDEX ${INDEX})
	endif()
	math(EXPR INDEX "${INDEX} + 1")
endforeach()
if(${SDLMAIN_INDEX} GREATER ${SDL_INDEX})
	list(REMOVE_AT SDL_LIBRARY ${SDLMAIN_INDEX})
	list(INSERT SDL_LIBRARY ${SDL_INDEX} ${SDLMAIN_LIBRARY})
endif()

ADD_DEFINITIONS(${SDL_LDFLAGS} ${SDL_CFLAGS})
ADD_DEFINITIONS(-I${SDL_INCLUDE_DIR})
if(MSVC)
	ADD_DEFINITIONS(-W4 -WX -wd"4996")
else()
	add_definitions(-fsigned-char -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)
	if(DEBUG)
		ADD_DEFINITIONS(-g)
		if(DEBUG_PROFILE)
			ADD_DEFINITIONS(-p)
		endif(DEBUG_PROFILE)
	else()
		ADD_DEFINITIONS(-O2)
	endif()
    if(CLANG)
    else()
        # Probably GCC...
        add_definitions(-freg-struct-return)
    endif()
endif()

include_directories(.)

set(CDOGS_DATA_DIR "")
set(CDOGS_CFG_DIR ".cdogs/")
IF(MSVC)
	SET(SOURCE_DIRECTORY "${CMAKE_BINARY_DIR}")
	SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cdogs-sdl")
ELSE()
	SET(SOURCE_DIRECTORY "..")
	SET(OUTPUT_DIRECTORY "../cdogs-sdl")
ENDIF()
# TODO: fix workaround due to path difference
IF(UNIX)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "..")
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.cmake ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl)
ENDIF()
IF(WIN32)
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/windows/cdogs.rc.cmake ${SOURCE_DIRECTORY}/build/windows/cdogs.rc)
ENDIF()
CONFIGURE_FILE(cdogs/sys_config.h.cmake ${SOURCE_DIRECTORY}/src/cdogs/sys_config.h)
CONFIGURE_FILE(${SOURCE_DIRECTORY}/README.md.cmake ${SOURCE_DIRECTORY}/README.md)

add_subdirectory(cdogs)
add_subdirectory(json)
add_subdirectory(tests)

set(CDOGS_SDL_SOURCES
	campaigns.c
	cdogs.c
	credits.c
	mainmenu.c
	menu.c
	password.c
	prep.c)
set(CDOGS_SDL_HEADERS
	campaigns.h
	credits.h
	mainmenu.h
	menu.h
	password.h
	prep.h
	missions/bem.h
	missions/ogre.h)
set(CDOGS_SDL_EXTRA)
if(WIN32)
	set(CDOGS_SDL_EXTRA ${SOURCE_DIRECTORY}/build/windows/cdogs.rc)
elseif(APPLE)
	set(CDOGS_SDL_EXTRA
		${SOURCE_DIRECTORY}/build/macosx/SDLMain.m
		${SOURCE_DIRECTORY}/build/macosx/SDLMain.h)
endif()
add_executable(cdogs-sdl ${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS} ${CDOGS_SDL_EXTRA})
target_link_libraries(cdogs-sdl cdogs json ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})

install(TARGETS cdogs-sdl DESTINATION .)

# Installation
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
	install(DIRECTORY ${SOURCE_DIRECTORY}/build/licenses DESTINATION .)
ENDIF()

# Packaging
SET(CPACK_PACKAGE_NAME "C-Dogs SDL")
SET(CPACK_PACKAGE_VENDOR "C-Dogs SDL Team")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C-Dogs SDL: Action/Arcade Game")
SET(CPACK_PACKAGE_VERSION ${VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_PACKAGE_EXECUTABLES "..\\\\cdogs-sdl;C-Dogs SDL")
SET(CPACK_RESOURCE_FILE_LICENSE ${SOURCE_DIRECTORY}/doc/COPYING.GPL)
IF(MSVC)
	SET(CPACK_GENERATOR NSIS)
	SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "C-Dogs SDL")
	SET(CPACK_NSIS_MUI_ICON ${SOURCE_DIRECTORY}/cdogs-icon.ico)
	SET(CPACK_CREATE_DESKTOP_LINKS cdogs-sdl)
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
	charsed.h)
add_executable(cdogs-sdl-editor ${CDOGS_SDL_EDITOR_SOURCES} ${CDOGS_SDL_EDITOR_HEADERS})
target_link_libraries(cdogs-sdl-editor cdogs json ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
