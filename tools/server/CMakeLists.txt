# set(VDB_SERVER_SOURCES vdb_server.cpp)
# add_executable(vdb_server ${VDB_SERVER_SOURCES})

# target_link_libraries(vdb_server vectorDB)
# set_target_properties(vdb_server PROPERTIES OUTPUT_NAME vdb_server)
# 定义源文件和目标名称的映射
set(EXECUTABLES
    vdb_server:vdb_server.cpp
    vdb_server_proxy:vdb_server_proxy.cpp
    # vdb_server_master:vdb_server_master.cpp
)

# 遍历每个目标，创建可执行文件并设置属性
foreach(EXECUTABLE_CONFIG ${EXECUTABLES})
    # 使用 ":" 分割目标名称和源文件
    string(REPLACE ":" ";" SPLIT_CONFIG ${EXECUTABLE_CONFIG})
    list(GET SPLIT_CONFIG 0 EXEC_NAME)
    list(GET SPLIT_CONFIG 1 SOURCE_FILE)

    # 创建可执行文件
    add_executable(${EXEC_NAME} ${SOURCE_FILE})
    
    # 链接到 vectorDB 库
    target_link_libraries(${EXEC_NAME} vectorDB)
    
    # 设置输出文件名
    set_target_properties(${EXEC_NAME} PROPERTIES OUTPUT_NAME ${EXEC_NAME})
endforeach()