cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0072 NEW)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
set(CMAKE_OSX_ARCHITECTURES x86_64)
project(cdogs-sdl C)

SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "2")
SET(VERSION_PATCH "0")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# Optionally configure CI files since they are excluded in source archives
if(EXISTS ${CMAKE_SOURCE_DIR}/.github/workflows/cmake.yml.cmake)
	CONFIGURE_FILE(
		${CMAKE_SOURCE_DIR}/.github/workflows/cmake.yml.cmake
		${CMAKE_SOURCE_DIR}/.github/workflows/cmake.yml
		@ONLY
	)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/appveyor.yml.cmake)
	CONFIGURE_FILE(
		${CMAKE_SOURCE_DIR}/appveyor.yml.cmake
		${CMAKE_SOURCE_DIR}/appveyor.yml
		@ONLY
	)
endif()

# this must be 4 numbers
SET(VERSION_RC "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")
SET(WEBSITE "http://cxong.github.io/cdogs-sdl/")

option(DEBUG "Enable debug build" OFF)
option(DEBUG_PROFILE "Enable debug profile build" OFF)
option(USE_SHARED_ENET "Use system installed copy of enet" OFF)
option(BUILD_EDITOR "Build cdogs-sdl-editor" ON)

# check for crosscompiling (defined when using a toolchain file)
if(CMAKE_CROSSCOMPILING)
	# check for gcw0
	if(CMAKE_C_COMPILER MATCHES ".*gcw0-linux.*")
		set(GCW0 1)
		set(BUILD_EDITOR OFF)
	endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CLANG 1)
endif()

if(WIN32)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	SET(CMAKE_RC_COMPILE_OBJECT
		"<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()
SET(LINKDIR /usr/games)

if(DEBUG)
	set(CMAKE_BUILD_TYPE "Debug")
else()
	add_definitions(-DNDEBUG)
endif()

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_SOURCE_DIR}/build/cmake"
	"${CMAKE_SOURCE_DIR}/src/proto/nanopb/extra")
	
find_package(Nanopb REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
if(BUILD_EDITOR)
	find_package(OpenGL REQUIRED)
endif()
find_package(CCache)
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS} ${SDL2_MIXER_LIBRARY_DIRS})

if(CCACHE_FOUND AND CLANG)
	add_definitions(-Qunused-arguments -fcolor-diagnostics)
endif()

SET(ENet_LIBRARY enet)
IF(WIN32)
    SET(WINDOWS_ENET_DEPENDENCIES "ws2_32;winmm")
    SET(ENet_LIBRARIES ${ENet_LIBRARY} ${WINDOWS_ENET_DEPENDENCIES})
ELSE()
    SET(ENet_LIBRARIES ${ENet_LIBRARY})
ENDIF()

IF(NOT USE_SHARED_ENET)
    INCLUDE_DIRECTORIES(src/cdogs/enet/include)
ENDIF()

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP -W4 -WX -wd\"4996\" -wd\"4204\"")
else()
	add_definitions(
		-fsigned-char
		-Wall -W
		-Wstrict-prototypes -Wpointer-arith -Wcast-qual)
	if(DEBUG)
		ADD_DEFINITIONS(-g)
		if(DEBUG_PROFILE AND CMAKE_COMPILER_IS_GNUCC)
			ADD_DEFINITIONS(-p)
		endif()
	else()
		ADD_DEFINITIONS(-O2)
	endif()
	if(GCW0)
		add_definitions(-D__GCWZERO__)
	endif()
    if(CLANG)
		add_definitions(-std=gnu99)
    else()
        # Probably GCC...
        add_definitions(-freg-struct-return -std=gnu99)
		if(NOT APPLE AND NOT BEOS AND NOT HAIKU)
			add_definitions(-Wshadow)
		endif()
		if (CMAKE_C_COMPILER_VERSION VERSION_GREATER 7)
			add_definitions(
				-Wimplicit-fallthrough=0 # TODO: is everything caught by implicit-fallthrough intended?
				-Wno-error=format-overflow # TODO: should probably just make sure buffer sizes are appropriate
			)
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
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
IF(UNIX AND NOT APPLE)
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/build/linux/cdogs-sdl.cmake ${CMAKE_SOURCE_DIR}/build/linux/cdogs-sdl)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/build/linux/cdogs-sdl PROPERTIES GENERATED TRUE)
ENDIF()
IF(WIN32)
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/build/windows/cdogs.rc.cmake ${CMAKE_SOURCE_DIR}/build/windows/cdogs.rc)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/build/windows/cdogs.rc PROPERTIES GENERATED TRUE)
ENDIF()
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/cdogs/sys_config.h.cmake ${CMAKE_SOURCE_DIR}/src/cdogs/sys_config.h)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/cdogs/sys_config.h PROPERTIES GENERATED TRUE)
include_directories(src src/cdogs)

# Tests
enable_testing()

add_subdirectory(src)

# Build the shared library next to cdogs-sdl
set_target_properties(cdogs-sdl PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
)
if(BUILD_EDITOR)
	set_target_properties(cdogs-sdl-editor PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/src
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/src
	)
endif()

################
# Installation #
################

# Executables
set(INSTALL_PREFIX ".")
set(EXE_EXTENSION "")
if(WIN32)
	set(EXE_EXTENSION ".exe")
	set(LIB_EXTENSION ".dll")
  set(DATA_INSTALL_DIR ".")
elseif(APPLE)
	set(LIB_EXTENSION ".dylib")
  set(DATA_INSTALL_DIR "cdogs-sdl")
	set(INSTALL_PREFIX "cdogs-sdl")
else()
	set(LIB_EXTENSION ".so")
	set(DATA_INSTALL_DIR ".")
endif()

# Since Debian wants games binaries in /usr/games
if(NOT DEFINED CDOGS_BIN_DIR)
	set(CDOGS_BIN_DIR "${INSTALL_PREFIX}/bin")
endif()

install(
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/src/cdogs-sdl${EXE_EXTENSION}
    DESTINATION ${CDOGS_BIN_DIR})
if(BUILD_EDITOR)
	install(
	  PROGRAMS
	    ${CMAKE_CURRENT_BINARY_DIR}/src/cdogs-sdl-editor${EXE_EXTENSION}
	    DESTINATION ${CDOGS_BIN_DIR})
endif()

INSTALL(DIRECTORY
	${CMAKE_SOURCE_DIR}/data
	${CMAKE_SOURCE_DIR}/missions
	${CMAKE_SOURCE_DIR}/dogfights
	${CMAKE_SOURCE_DIR}/graphics
	${CMAKE_SOURCE_DIR}/music
	${CMAKE_SOURCE_DIR}/sounds
	DESTINATION ${DATA_INSTALL_DIR})

if(NOT DEFINED CDOGS_DOC_DIR)
	set(CDOGS_DOC_DIR "${DATA_INSTALL_DIR}/doc")
endif()
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/
	DESTINATION ${CDOGS_DOC_DIR}
	)

INSTALL(FILES
	${CMAKE_SOURCE_DIR}/README.md
	DESTINATION ${DATA_INSTALL_DIR})
IF(UNIX AND NOT APPLE AND NOT BEOS AND NOT HAIKU)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/build/linux/io.github.cxong.cdogs-sdl.desktop DESTINATION ${INSTALL_PREFIX}/share/applications)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/build/linux/io.github.cxong.cdogs-sdl.appdata.xml DESTINATION ${INSTALL_PREFIX}/share/metainfo)
	foreach(RES 16 22 32 48 128)
		INSTALL(FILES ${CMAKE_SOURCE_DIR}/build/linux/cdogs-icon.${RES}.png
			DESTINATION ${INSTALL_PREFIX}/share/icons/hicolor/${RES}x${RES}/apps
			RENAME io.github.cxong.cdogs-sdl.png)
	endforeach(RES)
elseif(WIN32)
	# Package for Windows
	FILE(GLOB DLLS "${CMAKE_SOURCE_DIR}/dll/*.dll")
	FOREACH(DLL ${DLLS})
		INSTALL(FILES "${DLL}" DESTINATION ${INSTALL_PREFIX}/bin)
	ENDFOREACH()
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/build/licenses DESTINATION ${INSTALL_PREFIX})
elseif(APPLE)
	set(CMAKE_INSTALL_PREFIX "/Applications/cdogs-sdl")
endif()

# Packaging
SET(CPACK_PACKAGE_NAME "C-Dogs.SDL")
SET(CPACK_PACKAGE_VENDOR "C-Dogs SDL Team")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C-Dogs SDL: Action/Arcade Game")
SET(CPACK_PACKAGE_VERSION ${VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_PACKAGE_EXECUTABLES "cdogs-sdl;C-Dogs SDL;cdogs-sdl-editor;Campaign Editor")
if(WIN32)
	SET(CPACK_GENERATOR NSIS ZIP)
	SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/doc/COPYING.GPL)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "C-Dogs SDL")
	SET(CPACK_NSIS_MUI_ICON ${CMAKE_SOURCE_DIR}/build/windows/cdogs-icon.ico)
	set(CPACK_NSIS_CREATE_ICONS "SetOutPath '\$INSTDIR\\\\bin'
	CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\C-Dogs SDL.lnk' '\$INSTDIR\\\\bin\\\\cdogs-sdl.exe'
	CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Campaign Editor.lnk' '\$INSTDIR\\\\bin\\\\cdogs-sdl-editor.exe'")
	SET(CPACK_NSIS_MUI_FINISHPAGE_RUN cdogs-sdl.exe)
	SET(CPACK_NSIS_URL_INFO_ABOUT ${WEBSITE})
elseif(APPLE)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_FORMAT "UDBZ")
	set(CPACK_DMG_VOLUME_NAME "C-Dogs.SDL")
	set(CPACK_SYSTEM_NAME "OSX")
	set(CPACK_PACKAGE_ICON
		"${CMAKE_SOURCE_DIR}/build/macosx/cdogs-icon.icns")
	set(CPACK_DMG_BACKGROUND_IMAGE
		"${CMAKE_SOURCE_DIR}/build/macosx/dmg_bg.png")
	set(CPACK_DMG_DS_STORE
		"${CMAKE_SOURCE_DIR}/build/macosx/DS_Store")

	# copy SDL frameworks
	set(APPS cdogs-sdl cdogs-sdl-editor)
	set(LIBS SDL2 SDL2_image SDL2_mixer)
	foreach(APP ${APPS})
		foreach(LIB ${LIBS})
			install(DIRECTORY /Library/Frameworks/${LIB}.framework
				DESTINATION ${INSTALL_PREFIX}/Frameworks)
		endforeach()
	endforeach()
elseif(BEOS OR HAIKU)
	set(CPACK_GENERATOR TGZ)
	install(
		FILES ${CMAKE_SOURCE_DIR}/build/haiku/cdogs-sdl.sh
		DESTINATION ${INSTALL_PREFIX}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
	# probably linux

	# Pack only tar.gz since deb and rpm packages may differ in distros
	set(CPACK_GENERATOR TGZ)

	# debian specific
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "C-Dogs SDL Team")
	set(CPACK_DEBIAN_PACKAGE_SECTION Games)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libsdl2-image, libsdl2-mixer")

	# fedora/suse specific
	set(CPACK_RPM_PACKAGE_LICENSE "GPL2")
	set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games/Action/Arcade")
	set(CPACK_RPM_PACKAGE_REQUIRES "sdl_image >= 2, sdl_mixer >= 2")

endif()
include(CPack)
