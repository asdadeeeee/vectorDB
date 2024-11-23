add_library(
        vectorDB_logger
        OBJECT
        logger.cpp)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_logger>
        PARENT_SCOPE)