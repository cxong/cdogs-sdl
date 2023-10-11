# Copyright (c) 2007-2014, Lloyd Hilaiel <me@lloyd.io>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

set (YAJL_MAJOR 2)
set (YAJL_MINOR 1)
set (YAJL_MICRO 1)

set (SRCS yajl.c yajl_lex.c yajl_parser.c yajl_buf.c
          yajl_encode.c yajl_gen.c yajl_alloc.c
          yajl_tree.c yajl_version.c
)
set (HDRS yajl_parser.h yajl_lex.h yajl_buf.h yajl_encode.h yajl_alloc.h)
set (PUB_HDRS api/yajl_parse.h api/yajl_gen.h api/yajl_common.h api/yajl_tree.h)

# useful when fixing lexer bugs.
#add_definitions(-DYAJL_LEXER_DEBUG)

# Ensure defined when building YAJL (as opposed to using it from
# another project).  Used to ensure correct function export when
# building win32 DLL.
add_definitions(-DYAJL_BUILD)

# set up some paths
set (libDir ${CMAKE_CURRENT_BINARY_DIR}/../${YAJL_DIST_NAME}/lib)
set (incDir ${CMAKE_CURRENT_BINARY_DIR}/../${YAJL_DIST_NAME}/include/yajl)
set (shareDir ${CMAKE_CURRENT_BINARY_DIR}/../${YAJL_DIST_NAME}/share/pkgconfig)

# set the output path for libraries
set(LIBRARY_OUTPUT_PATH ${libDir})

add_library(yajl_s STATIC ${SRCS} ${HDRS} ${PUB_HDRS})

add_library(yajl SHARED ${SRCS} ${HDRS} ${PUB_HDRS})

#### setup shared library version number
set_target_properties(yajl PROPERTIES
                      DEFINE_SYMBOL YAJL_SHARED
                      SOVERSION ${YAJL_MAJOR}
                      VERSION ${YAJL_MAJOR}.${YAJL_MINOR}.${YAJL_MICRO})

#### ensure a .dylib has correct absolute installation paths upon installation
if(APPLE)
  set_target_properties(yajl PROPERTIES
                        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif(APPLE)

#### build up an sdk as a post build step

# create some directories
file(MAKE_DIRECTORY ${libDir})
file(MAKE_DIRECTORY ${incDir})

# generate build-time source
set(dollar $)
configure_file(api/yajl_version.h.cmake ${incDir}/yajl_version.h)
configure_file(yajl.pc.cmake ${shareDir}/yajl.pc)

# copy public headers to output directory
foreach (header ${PUB_HDRS})
  set (header ${CMAKE_CURRENT_SOURCE_DIR}/${header})

  exec_program(${CMAKE_COMMAND} ARGS -E copy_if_different ${header} ${incDir})

  add_custom_command(TARGET yajl_s POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header} ${incDir})
endforeach (header ${PUB_HDRS})

include_directories(${incDir}/..)

