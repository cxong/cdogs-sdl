include_directories(${SDL2_INCLUDE_DIRS})

file(READ gamecontrollerbuttondb.txt DB)
# Escape the newlines so it appears as multi line string in C
string(REPLACE "\n" "\\n\\\n" DB ${DB})
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/db.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/db.h)
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/db.h PROPERTIES GENERATED TRUE)

add_library(SDL_joystickbuttonnames STATIC
  SDL_joystickbuttonnames.c SDL_joystickbuttonnames.h)
target_link_libraries(SDL_joystickbuttonnames ${SDL2_LIBRARY})
