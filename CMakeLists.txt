cmake_minimum_required(VERSION 3.10)

project (VectorDB)

# set(CMAKE_C_FLAGS_DEBUG ON)
# set(CMAKE_CXX_FLAGS_DEBUG ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -g")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
message("CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)



set(VECTORDB_SRC_INCLUDE_DIR  ${PROJECT_SOURCE_DIR}/src/include)
set(VECTORDB_THIRD_PARTY_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party/installed/include)
set(PROTOS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/protocol)
add_compile_options(-Wno-unused-function)
include_directories(${VECTORDB_SRC_INCLUDE_DIR} ${VECTORDB_THIRD_PARTY_INCLUDE_DIR} ${PROTOS_INCLUDE_DIR})
string(REPLACE "-Wno-unused-function" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include_directories(BEFORE src) # This is needed for gtest.
enable_testing()

set(THIRD_PARTY_LIB)
set(PROTO_LIB)
add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(tools)