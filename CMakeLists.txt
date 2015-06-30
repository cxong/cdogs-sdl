if(APPLE)
	cmake_minimum_required(VERSION 2.8.12)
else()
	cmake_minimum_required(VERSION 2.8)
endif()
if("${CMAKE_VERSION}" VERSION_GREATER 3.1)
	cmake_policy(SET CMP0054 NEW)
endif()
project(cdogs-sdl C)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "5")
SET(VERSION_PATCH "8")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
# this must be 4 numbers
SET(VERSION_RC "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")
SET(WEBSITE "http://cxong.github.io/cdogs-sdl/")

set(DEBUG 0)			#set to 1 to enable debug build
set(DEBUG_PROFILE 0)	# set to 1 to enable debug profile build

# check for crosscompiling (defined when using a toolchain file)
if(CMAKE_CROSSCOMPILING)
	# check for gcw0
	if(CMAKE_C_COMPILER MATCHES ".*gcw0-linux.*")
		set(GCW0 1)
	endif()
endif()

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG 1)
endif()

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
	IF(UNIX AND NOT APPLE)
		SET(CMAKE_INSTALL_PREFIX /usr/local)
	ENDIF()
endif()
if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	SET(CMAKE_RC_COMPILE_OBJECT
		"<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	add_definitions(-mwindows)
endif()
SET(LINKDIR /usr/games)

if(DEBUG)
	set(CMAKE_BUILD_TYPE "Debug")
else()
	add_definitions(-DNDEBUG)
endif()

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_image REQUIRED)
set(USE_NET 1)

SET(ENet_LIBRARY enet)
IF(WIN32)
    SET(WINDOWS_ENET_DEPENDENCIES "ws2_32;winmm")
    SET(ENet_LIBRARIES ${ENet_LIBRARY} ${WINDOWS_ENET_DEPENDENCIES})
ELSE()
    SET(ENet_LIBRARIES ${ENet_LIBRARY})
ENDIF()
INCLUDE_DIRECTORIES(src/cdogs/enet/include)

execute_process(
	COMMAND "sh" "sdl-config" "--libs"
	OUTPUT_VARIABLE SDL_LDFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
	COMMAND "sh" "sdl-config" "--cflags"
	OUTPUT_VARIABLE SDL_CFLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
# Work-around for CMake SDL bug (need to link SDLmain before SDL)
set(SDLMAIN_INDEX 0)
set(SDL_INDEX 0)
set(SDLMAIN_LIBRARY "")
set(INDEX 0)
foreach(SDL_LIBRARY_ITEM ${SDL_LIBRARY})
	string(FIND ${SDL_LIBRARY_ITEM} "SDLmain" SDLMAIN_FIND_INDEX)
	string(FIND ${SDL_LIBRARY_ITEM} "SDL.dll" SDL_FIND_INDEX)
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

add_definitions(-DPB_FIELD_16BIT)	# Allow larger field numbers in nanopb
include_directories(src/cdogs/proto/nanopb)
if(MSVC)
	ADD_DEFINITIONS(-W4 -WX -wd"4996")
else()
	add_definitions(
		-fsigned-char
		-Wall -W
		-Wstrict-prototypes -Wpointer-arith -Wcast-qual)
	if(DEBUG)
		ADD_DEFINITIONS(-g)
		if(DEBUG_PROFILE)
			ADD_DEFINITIONS(-p)
		endif(DEBUG_PROFILE)
	else()
		ADD_DEFINITIONS(-O2)
	endif()
	if(GCW0)
		add_definitions(-D__GCWZERO__)
	endif()
    if(CLANG)
		add_definitions(-std=c99)
    else()
        # Probably GCC...
        add_definitions(-freg-struct-return -std=gnu99)
		if(NOT APPLE AND NOT BEOS AND NOT HAIKU)
			add_definitions(-Wshadow)
		endif()
	endif()
	if(NOT BEOS AND NOT HAIKU)
		add_definitions(-Winline -Werror)
		set(EXTRA_LIBRARIES "m")
	endif()
endif()

set(CMAKE_MACOSX_RPATH 1)

if(NOT DEFINED CDOGS_DATA_DIR)
	if(GCW0)
		set(CDOGS_DATA_DIR "./")
	else()
		set(CDOGS_DATA_DIR "../")
	endif()
endif()
message("Data dir is ${CDOGS_DATA_DIR}")
if(WIN32)
	set(CDOGS_CFG_DIR "C-Dogs SDL/")
else()
	set(CDOGS_CFG_DIR ".config/cdogs-sdl/")
ENDIF()
SET(SOURCE_DIRECTORY ".")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
IF(UNIX AND NOT APPLE)
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.cmake ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl)
	SET_SOURCE_FILES_PROPERTIES(${SOURCE_DIRECTORY}/build/linux/cdogs-sdl PROPERTIES GENERATED TRUE)
ENDIF()
IF(WIN32)
	CONFIGURE_FILE(${SOURCE_DIRECTORY}/build/windows/cdogs.rc.cmake ${SOURCE_DIRECTORY}/build/windows/cdogs.rc)
	SET_SOURCE_FILES_PROPERTIES(${SOURCE_DIRECTORY}/build/windows/cdogs.rc PROPERTIES GENERATED TRUE)
ENDIF()
CONFIGURE_FILE(${SOURCE_DIRECTORY}/src/cdogs/sys_config.h.cmake ${SOURCE_DIRECTORY}/src/cdogs/sys_config.h)
SET_SOURCE_FILES_PROPERTIES(${SOURCE_DIRECTORY}/src/cdogs/sys_config.h PROPERTIES GENERATED TRUE)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src/cdogs)

#Tests
enable_testing()

add_subdirectory(src)

# Build the shared library next to cdogs-sdl
set_target_properties(hqx PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
	LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
)
set_target_properties(cdogs-sdl PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
)
set_target_properties(cdogs-sdl-editor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
)

################
# Installation #
################

# Executables
set(INSTALL_PREFIX ".")
if(WIN32)
	set(EXE_EXTENSION ".exe")
	set(LIB_EXTENSION ".dll")
elseif(APPLE)
	set(LIB_EXTENSION ".dylib")
	set(INSTALL_PREFIX "cdogs-sdl")
else()
	set(EXE_EXTENSION "")
	set(LIB_EXTENSION ".so")
endif()
if(APPLE)
	install(
		DIRECTORY src/cdogs-sdl.app src/cdogs-sdl-editor.app
		DESTINATION ${INSTALL_PREFIX}/bin
		USE_SOURCE_PERMISSIONS)
else()
	install(
		PROGRAMS src/cdogs-sdl${EXE_EXTENSION} src/cdogs-sdl-editor${EXE_EXTENSION}
		DESTINATION ${INSTALL_PREFIX}/bin)
endif()

INSTALL(DIRECTORY
	${SOURCE_DIRECTORY}/doc
	${SOURCE_DIRECTORY}/data
	${SOURCE_DIRECTORY}/missions
	${SOURCE_DIRECTORY}/dogfights
	${SOURCE_DIRECTORY}/graphics
	${SOURCE_DIRECTORY}/music
	${SOURCE_DIRECTORY}/sounds
	DESTINATION ${INSTALL_PREFIX})
INSTALL(FILES
	${SOURCE_DIRECTORY}/cdogs_icon.bmp
	${SOURCE_DIRECTORY}/README.md
	DESTINATION ${INSTALL_PREFIX})
IF(UNIX AND NOT APPLE AND NOT BEOS AND NOT HAIKU)
	INSTALL(FILES ${SOURCE_DIRECTORY}/build/linux/cdogs-sdl.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	INSTALL(FILES ${SOURCE_DIRECTORY}/cdogs-sdl.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons)
elseif(WIN32)
	# Package for Windows
	FILE(GLOB DLLS "${SOURCE_DIRECTORY}/src/*.dll")
	FOREACH(DLL ${DLLS})
		INSTALL(FILES "${DLL}" DESTINATION ${INSTALL_PREFIX}/bin)
	ENDFOREACH()
	install(DIRECTORY ${SOURCE_DIRECTORY}/build/licenses DESTINATION ${INSTALL_PREFIX})
elseif(APPLE)
	set(CMAKE_INSTALL_PREFIX "/Applications/cdogs-sdl")
endif()

# Packaging
SET(CPACK_PACKAGE_NAME "C-Dogs SDL")
SET(CPACK_PACKAGE_VENDOR "C-Dogs SDL Team")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C-Dogs SDL: Action/Arcade Game")
SET(CPACK_PACKAGE_VERSION ${VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_PACKAGE_EXECUTABLES "cdogs-sdl;C-Dogs SDL;cdogs-sdl-editor;Campaign Editor")
if(WIN32)
	SET(CPACK_GENERATOR NSIS)
	SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_BINARY_DIR}/doc/COPYING.GPL)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "C-Dogs SDL")
	SET(CPACK_NSIS_MUI_ICON ${CMAKE_CURRENT_BINARY_DIR}/cdogs-icon.ico)
	set(CPACK_NSIS_CREATE_ICONS "SetOutPath '\$INSTDIR\\\\bin'
	CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\C-Dogs SDL.lnk' '\$INSTDIR\\\\bin\\\\cdogs-sdl.exe'
	CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Campaign Editor.lnk' '\$INSTDIR\\\\bin\\\\cdogs-sdl-editor.exe'")
	SET(CPACK_NSIS_MUI_FINISHPAGE_RUN cdogs-sdl.exe)
	SET(CPACK_NSIS_URL_INFO_ABOUT ${WEBSITE})
elseif(APPLE)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_FORMAT "UDBZ")
	set(CPACK_DMG_VOLUME_NAME "C-Dogs SDL")
	set(CPACK_SYSTEM_NAME "OSX")
	set(CPACK_PACKAGE_ICON
		"${CMAKE_CURRENT_BINARY_DIR}/build/macosx/cdogs-icon.icns")
	set(CPACK_DMG_BACKGROUND_IMAGE
		"${CMAKE_CURRENT_BINARY_DIR}/build/macosx/dmg_bg.png")
	set(CPACK_DMG_DS_STORE
		"${CMAKE_CURRENT_BINARY_DIR}/build/macosx/DS_Store")

	# copy SDL frameworks into app bundle for Mac OS X
	set(APPS cdogs-sdl cdogs-sdl-editor)
	set(LIBS SDL SDL_image SDL_mixer)
	foreach(APP ${APPS})
		foreach(LIB ${LIBS})
			install(DIRECTORY /Library/Frameworks/${LIB}.framework
				DESTINATION ${INSTALL_PREFIX}/bin/${APP}.app/Contents/Frameworks)
		endforeach()
	endforeach()
elseif(BEOS OR HAIKU)
	set(CPACK_GENERATOR TGZ)
	install(
		FILES ${SOURCE_DIRECTORY}/build/haiku/cdogs-sdl.sh
		DESTINATION ${INSTALL_PREFIX}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
	# probably linux
	set(CPACK_GENERATOR TGZ;DEB;RPM)

	# debian specific
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "C-Dogs SDL Team")
	set(CPACK_DEBIAN_PACKAGE_SECTION Games)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libsdl-image1.2, libsdl-mixer1.2")

	# fedora/suse specific
	set(CPACK_RPM_PACKAGE_LICENSE "GPL2")
	set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games/Action/Arcade")
	set(CPACK_RPM_PACKAGE_REQUIRES "sdl_image >= 1.2, sdl_mixer >= 1.2")

endif()
include(CPack)
