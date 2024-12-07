add_library(
        vectorDB_httpserver
        OBJECT
        http_server.cpp)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_httpserver>
        PARENT_SCOPE)