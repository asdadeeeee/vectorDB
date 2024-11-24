cmake_minimum_required(VERSION 3.10)

project (VectorDB)
message("CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(VECTORDB_SRC_INCLUDE_DIR  ${PROJECT_SOURCE_DIR}/src/include)
set(VECTORDB_THIRD_PARTY_INCLUDE_DIR
        ${PROJECT_SOURCE_DIR}/third_party
        ${PROJECT_SOURCE_DIR}/third_party/googletest-1.15.2/googletest/include
        ${PROJECT_SOURCE_DIR}/third_party/spdlog
        ${PROJECT_SOURCE_DIR}/third_party/backward
)
include_directories(${VECTORDB_SRC_INCLUDE_DIR} ${VECTORDB_THIRD_PARTY_INCLUDE_DIR})
include_directories(BEFORE src) # This is needed for gtest.
enable_testing()
add_subdirectory(src)
add_subdirectory(third_party)
add_subdirectory(test)