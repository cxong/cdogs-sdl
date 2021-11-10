# Note: build fails if this is placed within cdogs/ folder
include_directories(${NANOPB_INCLUDE_DIRS})

add_definitions(-DPB_FIELD_16BIT)	# Allow larger field numbers in nanopb
# Note: uncomment below line to build new proto files
# nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS msg.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)
add_library(cdogs_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS} msg.options msg.proto)
