include_directories(
	.
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS})
if(NOT "${GCW0}")
	include_directories(${OPENGL_INCLUDE_DIRS})
endif()

add_subdirectory(cdogs)
if(NOT "${GCW0}")
	add_subdirectory(cdogsed)
endif()
add_subdirectory(json)
add_subdirectory(tests)

set(CDOGS_SDL_SOURCES
	autosave.c
	briefing_screens.c
	cdogs.c
	command_line.c
	credits.c
	game.c
	mainmenu.c
	menu.c
	menu_utils.c
	namegen.c
	password.c
	player_select_menus.c
	prep.c
	screens.c
	screens_end.c
	weapon_menu.c
	XGetopt.c)
set(CDOGS_SDL_HEADERS
	autosave.h
	briefing_screens.h
	command_line.h
	credits.h
	game.h
	mainmenu.h
	menu.h
	menu_utils.h
	namegen.h
	password.h
	player_select_menus.h
	prep.h
	screens.h
	screens_end.h
	weapon_menu.h
	XGetopt.h)
set(CDOGS_SDL_EXTRA)
if(WIN32)
	set(CDOGS_SDL_EXTRA ../build/windows/cdogs.rc)
elseif(APPLE)
	set(EXTRA_LIBRARIES "${EXTRA_LIBRARIES} -framework Carbon -framework IOKit")
endif()
add_executable(cdogs-sdl
	${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS} ${CDOGS_SDL_EXTRA})
if(APPLE)
	set_target_properties(cdogs-sdl PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks")
elseif(MSVC)
	set_target_properties(cdogs-sdl PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
endif()
target_link_libraries(cdogs-sdl cdogs ${EXTRA_LIBRARIES})

if(GCW0)
	add_custom_command(TARGET cdogs-sdl
		POST_BUILD
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../build/gcw-zero/make_opk.sh ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
		COMMENT "Building cdogs-sdl.opk..."
	)
endif()

if(NOT "${GCW0}")
  add_executable(cdogs-sdl-editor cdogsed.c ${CDOGS_SDL_EXTRA})
  if(APPLE)
  	set_target_properties(cdogs-sdl-editor PROPERTIES
  		MACOSX_RPATH 1
  		BUILD_WITH_INSTALL_RPATH 1
  		INSTALL_RPATH "@loader_path/../Frameworks")
  elseif(MSVC)
  	set_target_properties(cdogs-sdl-editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
  endif()
  target_link_libraries(cdogs-sdl-editor cdogsedlib cdogs ${OPENGL_LIBRARIES} ${EXTRA_LIBRARIES})
endif()
