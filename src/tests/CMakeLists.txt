if(MSVC)
	add_definitions(-wd"4127" -wd"4102")
else()
	add_definitions(-Wno-unused-label)
	set(EXTRA_LIBRARIES "m")
endif()

add_subdirectory(cbehave)

include_directories(. ../cdogs)

add_executable(config_test
	config_test.c
	../cdogs/config.h
	../cdogs/config.c
	../cdogs/config_json.c
	../cdogs/config_json.h
	../cdogs/config_old.c
	../cdogs/config_old.h
	../cdogs/utils.c
	../cdogs/utils.h)
target_link_libraries(config_test cbehave json ${EXTRA_LIBRARIES})
