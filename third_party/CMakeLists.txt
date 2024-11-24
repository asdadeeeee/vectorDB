
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # don't override our compiler/linker options when building gtest
add_subdirectory(googletest-1.15.2)
add_subdirectory(backward-cpp)


# set(FORT_ENABLE_TESTING OFF CACHE INTERNAL "")

