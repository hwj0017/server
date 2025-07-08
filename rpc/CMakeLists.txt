
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
add_library(rpc STATIC ${SRC})
message(STATUS "SRC: ${SRC}")

target_include_directories(rpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tcp/include ${PROJECT_SOURCE_DIR}/utils/include)
target_link_libraries(rpc PUBLIC tcp)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test/service service)
add_executable(rpcserver ${service} test/server.cpp)
target_include_directories(rpcserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/utils/include)
target_link_libraries(rpcserver PUBLIC rpc protobuf)

add_executable(rpcclient ${service} test/client.cpp)
target_include_directories(rpcclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/utils/include)
target_link_libraries(rpcclient PUBLIC rpc protobuf)