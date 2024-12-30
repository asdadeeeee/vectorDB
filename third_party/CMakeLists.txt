add_library(third_party_lib INTERFACE)

set(STATIC_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/installed/lib)

set(LIB_LIST
    faiss
    openblas
    brpc
    gflags
    protobuf
    glog
    crypto
    leveldb
    ssl
    z
    rocksdb
    snappy
    lz4
    bz2
    roaring
)

foreach(lib_name IN LISTS LIB_LIST)
    set(lib_path ${STATIC_LIB_DIR}/lib${lib_name}.a)
    message(STATUS "Importing ${lib_name} ${lib_path}")
    add_library(${lib_name} STATIC IMPORTED)
    set_target_properties(${lib_name} PROPERTIES IMPORTED_LOCATION ${lib_path})

    target_link_libraries(third_party_lib INTERFACE ${lib_name})
endforeach()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/googletest-1.15.2)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/backward-cpp-1.6)
set(THIRD_PARTY_LIB ${THIRD_PARTY_LIB} third_party_lib PARENT_SCOPE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_LIB ${PROTO_LIB} PARENT_SCOPE)

