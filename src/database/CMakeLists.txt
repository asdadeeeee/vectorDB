add_library(
        vectorDB_database
        OBJECT
        scalar_storage.cpp
        vector_database.cpp
        )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_database>
        PARENT_SCOPE)