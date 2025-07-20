
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
add_library(tcp STATIC ${SRC})
target_include_directories(tcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/utils/include)

add_executable(client test/client.cpp)
target_link_libraries(client tcp)
target_include_directories(client PRIVATE ${PROJECT_SOURCE_DIR}/utils/include)

add_executable(echoserver test/echoserver.cpp)
target_link_libraries(echoserver tcp)
target_include_directories(echoserver PRIVATE ${PROJECT_SOURCE_DIR}/utils/include)

add_executable(vpnserver test/vpnserver.cpp)
target_link_libraries(vpnserver tcp)
target_include_directories(vpnserver PRIVATE ${PROJECT_SOURCE_DIR}/utils/include)