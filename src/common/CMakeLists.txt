add_library(
        vectorDB_common
        OBJECT
        vector_cfg.cpp
        proxy_cfg.cpp
        master_cfg.cpp
        vector_init.cpp
        )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_common>
        PARENT_SCOPE)