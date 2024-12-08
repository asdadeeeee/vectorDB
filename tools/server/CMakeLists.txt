set(VDB_SERVER_SOURCES vdb_server.cpp)
add_executable(vdb_server ${VDB_SERVER_SOURCES})

target_link_libraries(vdb_server vectorDB)
set_target_properties(vdb_server PROPERTIES OUTPUT_NAME vdb_server)