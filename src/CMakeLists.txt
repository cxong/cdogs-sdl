include_directories(.)

add_subdirectory(cdogs)
add_subdirectory(json)
add_subdirectory(tests)

set(CDOGS_SDL_SOURCES
	autosave.c
	cdogs.c
	credits.c
	game.c
	mainmenu.c
	menu.c
	password.c
	prep.c
	XGetopt.c)
set(CDOGS_SDL_HEADERS
	autosave.h
	credits.h
	game.h
	mainmenu.h
	menu.h
	password.h
	prep.h
	XGetopt.h
	missions/bem.h
	missions/ogre.h)
set(CDOGS_SDL_EXTRA)
if(WIN32)
	set(CDOGS_SDL_EXTRA ../build/windows/cdogs.rc)
elseif(APPLE)
	set(CDOGS_SDL_EXTRA
		../build/macosx/cdogs-icon.icns
		../build/macosx/SDLMain.m
		../build/macosx/SDLMain.h)
	set_source_files_properties(../build/macosx/SDLMain.m
		PROPERTIES LANGUAGE C)
endif()
add_executable(cdogs-sdl ${CDOGS_SDL_SOURCES} ${CDOGS_SDL_HEADERS} ${CDOGS_SDL_EXTRA})
target_link_libraries(cdogs-sdl cdogs json hqx ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${EXTRA_LIBRARIES})

SET(CDOGS_SDL_EDITOR_SOURCES
	cdogsed.c
	charsed.c)
SET(CDOGS_SDL_EDITOR_HEADERS
	charsed.h)
add_executable(cdogs-sdl-editor ${CDOGS_SDL_EDITOR_SOURCES} ${CDOGS_SDL_EDITOR_HEADERS} ${CDOGS_SDL_EXTRA})
target_link_libraries(cdogs-sdl-editor cdogs json hqx ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${EXTRA_LIBRARIES})