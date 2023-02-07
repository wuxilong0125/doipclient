cmake_minimum_required(VERSION 2.8)
set(PROJECT_NAME "doip_test")
project(${PROJECT_NAME} CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -std=c++11 -pthread")

aux_source_directory(../src SRC_LIST)
aux_source_directory(./ TEST_LIST)
add_executable(${PROJECT_NAME} ${TEST_LIST} ${SRC_LIST})


