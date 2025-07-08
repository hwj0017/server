
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
add_library(tcp STATIC ${SRC})
target_include_directories(tcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/utils/include)