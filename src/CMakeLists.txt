cmake_minimum_required(VERSION 3.10)

add_subdirectory(index)
add_subdirectory(logger)
add_subdirectory(httpserver)
add_subdirectory(database)
add_subdirectory(common)
add_subdirectory(cluster)
add_library(vectorDB STATIC ${ALL_OBJECT_FILES})

set(VECTORDB_LIBS
        vectorDB_index
        vectorDB_logger
        vectorDB_httpserver
        vectorDB_database
        vectorDB_common
        vectorDB_cluster
        )


set(WL_START_GROUP "-Wl,--start-group")
set(WL_END_GROUP "-Wl,--end-group")
set(LINK_LIBS ${LINK_LIBS}
        ${WL_START_GROUP}
        ${THIRD_PARTY_LIB}
        ${WL_END_GROUP}
        ${PROTO_LIB}
        ${VECTORDB_LIBS}
        -fopenmp
        -lpthread
        -lstdc++fs
        -lzstd
        -lcpprest
        -lgfortran
        -ldl
    )

message(STATUS "Link_libs: ${LINK_LIBS}")
target_link_libraries(vectorDB ${LINK_LIBS})

target_include_directories(
        vectorDB PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
