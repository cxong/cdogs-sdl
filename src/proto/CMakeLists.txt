# Note: build fails if this is placed within cdogs/ folder
include_directories(${NANOPB_INCLUDE_DIRS})

add_definitions(-DPB_FIELD_16BIT)	# Allow larger field numbers in nanopb

# Note: uncomment below line to build new proto files
# nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS msg.proto)
set(PROTO_SRCS "msg.pb.c" "nanopb/pb_decode.c" "nanopb/pb_encode.c" "nanopb/pb_common.c")
set(PROTO_HDRS "msg.pb.h" "nanopb/pb_decode.h" "nanopb/pb_encode.c" "nanopb/pb_common.c" "nanopb/pb.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(cdogs_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS} msg.options msg.proto)
