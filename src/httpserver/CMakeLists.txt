add_library(
        vectorDB_httpserver
        OBJECT
        http_server.cpp
        proxy_server.cpp
        proxy_service_impl.cpp
        user_service_impl.cpp
        admin_service_impl.cpp
        base_service_impl.cpp)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_httpserver>
        PARENT_SCOPE)