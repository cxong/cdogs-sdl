if(MSVC)
	add_definitions(-wd"4127" -wd"4102")
else()
	if(NOT BEOS AND NOT HAIKU)
		add_definitions(-Wno-unused-label)
		set(EXTRA_LIBRARIES "m")
	endif()
endif()

add_subdirectory(cbehave)

include_directories(. ../cdogs ${SDL_INCLUDE_DIR})

add_executable(autosave_test
	autosave_test.c
	../autosave.h
	../autosave.c
	../cdogs/color.c
	../cdogs/json_utils.c
	../cdogs/json_utils.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(autosave_test cbehave json ${EXTRA_LIBRARIES})

add_executable(color_test
	color_test.c
	../cdogs/color.c
	../cdogs/color.h)
target_link_libraries(color_test cbehave ${EXTRA_LIBRARIES})

add_executable(config_test
	config_test.c
	../cdogs/color.c
	../cdogs/config.h
	../cdogs/config.c
	../cdogs/config_json.c
	../cdogs/config_json.h
	../cdogs/config_old.c
	../cdogs/config_old.h
	../cdogs/json_utils.c
	../cdogs/json_utils.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(config_test cbehave json ${EXTRA_LIBRARIES})
