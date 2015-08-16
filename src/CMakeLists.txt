include_directories(. ${SDL_INCLUDE_DIR})

add_subdirectory(cdogs)
add_subdirectory(cdogsed)
add_subdirectory(json)
add_subdirectory(tests)

set(CDOGS_SDL_SOURCES
	autosave.c
	briefing_screens.c
	cdogs.c
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
	weapon_menu.c
	XGetopt.c)
set(CDOGS_SDL_HEADERS
	autosave.h
	briefing_screens.h
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
	weapon_menu.h
	XGetopt.h)
set(CDOGS_SDL_EXTRA)
if(WIN32)
	set(CDOGS_SDL_EXTRA ../build/windows/cdogs.rc)
elseif(APPLE)
	set(CDOGS_SDL_EXTRA
		../build/macosx/cdogs-icon.icns
		../build/macosx/SDLMain.m
		../build/macosx/SDLMain.h)
	set_source_files_properties(../build/macosx/cdogs-icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set(MACOSX_BUNDLE_ICON_FILE cdogs-icon.icns)
	set_source_files_properties(../build/macosx/SDLMain.m
		PROPERTIES LANGUAGE C)
	set(EXTRA_LIBRARIES "${EXTRA_LIBRARIES} -framework Carbon -framework IOKit")
endif()
add_executable(cdogs-sdl MACOSX_BUNDLE ${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS} ${CDOGS_SDL_EXTRA})
if(APPLE)
	set_target_properties(cdogs-sdl PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks")
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

add_executable(cdogs-sdl-editor MACOSX_BUNDLE cdogsed.c ${CDOGS_SDL_EXTRA})
if(APPLE)
	set_target_properties(cdogs-sdl-editor PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks")
endif()
target_link_libraries(cdogs-sdl-editor cdogsedlib cdogs ${EXTRA_LIBRARIES})
