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
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS})

add_executable(autosave_test
	autosave_test.c
	../autosave.h
	../autosave.c
	../cdogs/campaign_entry.c
	../cdogs/c_array.c
	../cdogs/color.c
	../cdogs/json_utils.c
	../cdogs/json_utils.h
	../cdogs/log.c
	../cdogs/log.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(autosave_test
	cbehave
	json
	${SDL2_LIBRARY}
	${EXTRA_LIBRARIES})
add_test(NAME autosave_test COMMAND autosave_test)

add_executable(c_hashmap_test
	c_hashmap_test.c
	../cdogs/c_hashmap/hashmap.h
	../cdogs/c_hashmap/hashmap.c)
target_link_libraries(c_hashmap_test cbehave ${EXTRA_LIBRARIES})
add_test(NAME c_hashmap_test COMMAND c_hashmap_test)

add_executable(c_array_test
	c_array_test.c
	../cdogs/c_array.h
	../cdogs/c_array.c
	../cdogs/color.c
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(c_array_test
	cbehave
	${SDL2_LIBRARY} ${EXTRA_LIBRARIES})
add_test(NAME c_array_test COMMAND c_array_test)

add_executable(color_test
	color_test.c
	../cdogs/color.c
	../cdogs/color.h)
target_link_libraries(color_test cbehave ${EXTRA_LIBRARIES})
add_test(NAME color_test COMMAND color_test)

add_executable(config_test
	config_test.c
	../cdogs/c_array.h
	../cdogs/c_array.c
	../cdogs/color.c
	../cdogs/color.h
	../cdogs/config.c
	../cdogs/config.h
	../cdogs/config_io.c
	../cdogs/config_io.h
	../cdogs/config_json.c
	../cdogs/config_json.h
	../cdogs/config_old.c
	../cdogs/config_old.h
	../cdogs/json_utils.c
	../cdogs/json_utils.h
	../cdogs/log.c
	../cdogs/log.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(config_test
	cbehave
	json
	${SDL2_LIBRARY}
	${EXTRA_LIBRARIES})
add_test(NAME config_test COMMAND config_test)

add_executable(json_test
	json_test.c
	../cdogs/c_array.h
	../cdogs/c_array.c
	../cdogs/color.h
	../cdogs/color.c
	../cdogs/json_utils.c
	../cdogs/json_utils.h
	../cdogs/log.c
	../cdogs/log.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(json_test
	cbehave
	json
	${SDL2_LIBRARY}
	${EXTRA_LIBRARIES})
add_test(NAME json_test COMMAND json_test)

add_executable(minkowski_hex_test
	minkowski_hex_test.c
	../cdogs/collision/minkowski_hex.c
	../cdogs/collision/minkowski_hex.h
	../cdogs/utils.c
	../cdogs/utils.h
	../cdogs/vector.c
	../cdogs/vector.h)
target_link_libraries(minkowski_hex_test
	cbehave
	${EXTRA_LIBRARIES})
add_test(NAME minkowski_hex_test COMMAND minkowski_hex_test)

add_executable(pic_test
	pic_test.c
	../cdogs/c_array.c
	../cdogs/c_array.h
	../cdogs/color.c
	../cdogs/color.h
	../cdogs/config.c
	../cdogs/config.h
	../cdogs/grafx.c
	../cdogs/grafx.h
	../cdogs/log.c
	../cdogs/log.h
	../cdogs/pic.c
	../cdogs/pic.h
	../cdogs/utils.c
	../cdogs/utils.h
	../cdogs/vector.c
	../cdogs/vector.h)
target_link_libraries(pic_test
	cbehave
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARIES}
	${EXTRA_LIBRARIES})
# TODO: test disabled since Travis-CI fails with "No available video device"
#add_test(NAME pic_test COMMAND pic_test)

add_executable(player_test
	player_test.c
	../cdogs/c_array.c
	../cdogs/c_array.h
	../cdogs/color.c
	../cdogs/color.h
	../cdogs/log.c
	../cdogs/log.h
	../cdogs/player.c
	../cdogs/player.h
	../cdogs/vector.c
	../cdogs/vector.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(player_test
	cbehave
	${SDL2_LIBRARY}
	${EXTRA_LIBRARIES})
add_test(NAME player_test COMMAND player_test)

add_executable(utils_test
	utils_test.c
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(utils_test
	cbehave
	${SDL2_LIBRARY} ${EXTRA_LIBRARIES})
add_test(NAME utils_test COMMAND utils_test)
