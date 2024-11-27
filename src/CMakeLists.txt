cmake_minimum_required(VERSION 3.10)

add_subdirectory(index)
add_subdirectory(logger)
add_library(vectorDB STATIC ${ALL_OBJECT_FILES})

set(VECTORDB_LIBS
        vectorDB_index
        vectorDB_logger
        )

target_link_libraries(
    vectorDB
    ${VECTORDB_LIBS})

target_include_directories(
        vectorDB PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

find_package(Faiss)
        if(Faiss_FOUND)
            target_include_directories(vectorDB PRIVATE ${Faiss_INCLUDE_DIR})
            target_link_libraries(vectorDB ${Faiss_LIBRARY})
        else(Faiss_FOUND)
            message(FATAL_ERROR ”Faiss library not found”)
        endif(Faiss_FOUND)