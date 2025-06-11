cmake_minimum_required(VERSION 3.18)
project(MyProjectCommonSubproject LANGUAGES CXX)

find_package(Drogon CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(libopaque CONFIG REQUIRED)

set(PROTO_INCLUDE_DIR "${CMAKE_BINARY_DIR}/gen_include")
set(PROTO_OUT_DIR "${PROTO_INCLUDE_DIR}/common/proto")
file(MAKE_DIRECTORY "${PROTO_OUT_DIR}")

add_custom_command(
  OUTPUT "${PROTO_OUT_DIR}/chat.pb.h" "${PROTO_OUT_DIR}/chat.pb.cc"
  COMMAND protobuf::protoc
    --cpp_out="${PROTO_OUT_DIR}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/protobuf"
    "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/chat.proto"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/chat.proto"
)

add_library(common_lib STATIC
  ${PROTO_OUT_DIR}/chat.pb.cc
)

target_include_directories(common_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROTO_INCLUDE_DIR}
)

target_link_libraries(common_lib PUBLIC Drogon::Drogon protobuf::libprotobuf libopaque)
