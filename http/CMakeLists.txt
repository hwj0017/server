aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
add_library(http STATIC ${SRC})

add_executable(httpservertest test/servertest.cpp)
add_executable(httpclienttest test/clienttest.cpp)
target_link_libraries(httpservertest http)
target_link_libraries(http tcp)
target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tcp/include ${PROJECT_SOURCE_DIR}/utils/include)