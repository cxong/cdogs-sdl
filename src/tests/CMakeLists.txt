if(MSVC)
	add_definitions(-wd"4127" -wd"4102")
else()
	if(NOT BEOS AND NOT HAIKU)
		add_definitions(-Wno-unused-label)
		set(EXTRA_LIBRARIES "m")
	endif()
endif()

add_subdirectory(cbehave)

include_directories(
	. ../cdogs
	${SDL2_INCLUDE_DIRS})

add_executable(animated_counter_test
	animated_counter_test.c
	../animated_counter.h
	../animated_counter.c)
target_link_libraries(animated_counter_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME animated_counter_test COMMAND animated_counter_test)
if(APPLE)
	set_target_properties(animated_counter_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(actor_test
	actor_test.c
	../cdogs/actors.h
	../cdogs/actors.c)
target_link_libraries(actor_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME actor_test COMMAND actor_test)
if(APPLE)
	set_target_properties(actor_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(autosave_test
	autosave_test.c
	../autosave.h
	../autosave.c)
target_link_libraries(autosave_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME autosave_test COMMAND autosave_test)
if(APPLE)
	set_target_properties(autosave_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(c_hashmap_test
	c_hashmap_test.c
	../cdogs/c_hashmap/hashmap.h
	../cdogs/c_hashmap/hashmap.c)
target_link_libraries(c_hashmap_test cbehave ${EXTRA_LIBRARIES})
add_test(NAME c_hashmap_test COMMAND c_hashmap_test)

add_executable(c_array_test
	c_array_test.c
	../cdogs/c_array.h
	../cdogs/c_array.c)
target_link_libraries(c_array_test
	cbehave ${EXTRA_LIBRARIES})
add_test(NAME c_array_test COMMAND c_array_test)

add_executable(color_test
	color_test.c
	../cdogs/color.c
	../cdogs/color.h)
target_link_libraries(color_test cbehave ${EXTRA_LIBRARIES})
add_test(NAME color_test COMMAND color_test)

add_executable(config_test config_test.c)
target_link_libraries(config_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME config_test COMMAND config_test)
if(APPLE)
	set_target_properties(config_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(json_test json_test.c)
target_link_libraries(json_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME json_test COMMAND json_test)
if(APPLE)
	set_target_properties(json_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(minkowski_hex_test minkowski_hex_test.c)
target_link_libraries(minkowski_hex_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME minkowski_hex_test COMMAND minkowski_hex_test)
if(APPLE)
	set_target_properties(minkowski_hex_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(pic_test pic_test.c)
target_link_libraries(pic_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	SDL2_image::SDL2_image
	${EXTRA_LIBRARIES})
# TODO: test disabled since CI fails with Couldn't find matching render driver
#add_test(NAME pic_test COMMAND pic_test)
if(APPLE)
	set_target_properties(pic_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(player_test player_test.c)
target_link_libraries(player_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2
	${EXTRA_LIBRARIES})
add_test(NAME player_test COMMAND player_test)
if(APPLE)
	set_target_properties(player_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(utils_test utils_test.c)
target_link_libraries(utils_test
	cbehave
	cdogs
	cdogs_proto
	SDL2::SDL2 ${EXTRA_LIBRARIES})
add_test(NAME utils_test COMMAND utils_test)
if(APPLE)
	set_target_properties(utils_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()
