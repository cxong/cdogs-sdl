if(MSVC)
	add_definitions(-wd"4127" -wd"4102")
endif()

add_subdirectory(cbehave)

include_directories(. ../cdogs)

add_executable(config_test
	config_test.c
	../cdogs/config.h
	../cdogs/config.c
	../cdogs/config_json.h
	../cdogs/config_json.c)
target_link_libraries(config_test cbehave json)