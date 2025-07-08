cmake_minimum_required(VERSION 3.10)
project(rpcserver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

aux_source_directory(src SRC_LIST)
add_executable(rpcserver ${SRC_LIST} server.cpp)
add_executable(rpcclient ${SRC_LIST} client.cpp)

target_link_libraries(rpcserver protobuf)
target_link_libraries(rpcclient protobuf)