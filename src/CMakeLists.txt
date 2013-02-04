cmake_minimum_required(VERSION 2.6)
project(cdogs-sdl C)

set(DEBUG 0)			#set to 1 to enable debug build
set(DEBUG_PROFILE 0)	# set to 1 to enable debug profile build

if(DEBUG)
	set(CMAKE_BUILD_TYPE "Debug")
endif(DEBUG)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "..")

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)

if(APPLE)
	ADD_DEFINITIONS(-DSYS_MACOSX)
endif(APPLE)
if(WINDOWS)
	ADD_DEFINITIONS(-DSYS_WIN)
endif(WINDOWS)

ADD_DEFINITIONS(-DSND_SDLMIXER -lSDL_mixer)


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
else(MSVC)
	ADD_DEFINITIONS(-fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)
	if(DEBUG)
		ADD_DEFINITIONS(-g)
		if(DEBUG_PROFILE)
			ADD_DEFINITIONS(-p)
		endif(DEBUG_PROFILE)
	else(DEBUG)
		ADD_DEFINITIONS(-O2)
	endif(DEBUG)
endif(MSVC)

include_directories(include missions tinydir)

set(CDOGS_DATA_DIR ".")
set(CDOGS_CFG_DIR ".cdogs/")
set(CDOGS_TEMP_DIR "/tmp/cdogs/")
if(WIN32)
	set(CDOGS_TEMP_DIR $ENV{TMP})
endif(WIN32)
CONFIGURE_FILE(config.h.cmake config.h)

add_library(cdogs STATIC
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

add_executable(cdogs-sdl
	cdogs.c
	drawtools.c
	mainmenu.c
	menu.c
	password.c
	prep.c)
target_link_libraries(cdogs-sdl cdogs SDL)
if(SDLMIXER_INCLUDE_DIR)
	target_link_libraries(cdogs-sdl SDL_mixer)
endif(SDLMIXER_INCLUDE_DIR)

add_executable(cdogs-sdl-editor
	cdogsed.c
	charsed.c)
target_link_libraries(cdogs-sdl-editor cdogs SDL)
if(SDLMIXER_INCLUDE_DIR)
	target_link_libraries(cdogs-sdl-editor SDL_mixer)
endif(SDLMIXER_INCLUDE_DIR)