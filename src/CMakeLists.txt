include_directories(
	.
	proto/nanopb
	${SDL2_INCLUDE_DIRS})
if(BUILD_EDITOR)
	include_directories(${OPENGL_INCLUDE_DIRS})
endif()

add_subdirectory(cdogs)
if(BUILD_EDITOR)
	add_subdirectory(cdogsed)
endif()
add_subdirectory(json)
add_subdirectory(proto)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

set(CDOGS_SDL_SOURCES
	ammo_menu.c
	animated_counter.c
	autosave.c
	briefing_screens.c
	cdogs.c
	command_line.c
	credits.c
	equip_menu.c
	game.c
	game_loop.c
	hiscores.c
	loading_screens.c
	mainmenu.c
	menu.c
	menu_utils.c
	namegen.c
	options_menu.c
	password.c
	pause_menu.c
	player_select_menus.c
	prep.c
	prep_equip.c
	screens_end.c
	util_menu.c
	weapon_menu.c)
set(CDOGS_SDL_HEADERS
	ammo_menu.h
	animated_counter.h
	autosave.h
	briefing_screens.h
	command_line.h
	credits.h
	equip_menu.h
	game.h
	game_loop.h
	hiscores.h
	loading_screens.h
	mainmenu.h
	menu.h
	menu_utils.h
	namegen.h
	options_menu.h
	password.h
	pause_menu.h
	player_select_menus.h
	prep.h
	prep_equip.h
	screens_end.h
	util_menu.h
	weapon_menu.h)
set(CDOGS_SDL_EXTRA)
if(WIN32)
	set(CDOGS_SDL_EXTRA ../build/windows/cdogs.rc)
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
target_link_libraries(cdogs-sdl cdogs cdogs_proto ${EXTRA_LIBRARIES})

if(GCW0)
	add_custom_command(TARGET cdogs-sdl
		POST_BUILD
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../build/gcw-zero/make_opk.sh ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
		COMMENT "Building cdogs-sdl.opk..."
	)
endif()

if(BUILD_EDITOR)
  add_executable(cdogs-sdl-editor cdogsed/cdogsed.c ${CDOGS_SDL_EXTRA})
  if(APPLE)
  	set_target_properties(cdogs-sdl-editor PROPERTIES
  		MACOSX_RPATH 1
  		BUILD_WITH_INSTALL_RPATH 1
  		INSTALL_RPATH "@loader_path/../Frameworks")
  elseif(MSVC)
  	set_target_properties(cdogs-sdl-editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
  endif()
  target_link_libraries(cdogs-sdl-editor cdogsedlib cdogs cdogs_proto ${OPENGL_LIBRARIES} ${EXTRA_LIBRARIES})
endif()
