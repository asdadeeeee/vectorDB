add_library(
        vectorDB_cluster
        OBJECT
        # in_memory_log_store.cpp
        log_state_machine.cpp
        # raft_stuff.cpp                                                                                 
        )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vectorDB_database>
        PARENT_SCOPE)