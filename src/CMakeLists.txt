cmake_minimum_required(VERSION 3.10)

add_subdirectory(index)
add_subdirectory(logger)
add_subdirectory(httpserver)
add_library(vectorDB STATIC ${ALL_OBJECT_FILES})

set(VECTORDB_LIBS
        vectorDB_index
        vectorDB_logger
        vectorDB_httpserver
        )



FIND_PACKAGE(BLAS REQUIRED)
if(BLAS_FOUND)
    message("BLAS FOUND")
endif()

FIND_PACKAGE( OpenMP REQUIRED)
if(OPENMP_FOUND)
    message("OPENMP FOUND")
endif()


set(VECTORDB_THIRDPARTY_LIBS
    faiss 
    blas
    OpenMP::OpenMP_CXX
    )


target_link_libraries(
    vectorDB
    ${VECTORDB_LIBS}
    ${VECTORDB_THIRDPARTY_LIBS})

target_include_directories(
        vectorDB PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
