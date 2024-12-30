file(GLOB PROTO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

set(PROTO_CC_FILES)

foreach(proto_file ${PROTO_SRC_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WE)
    set(pb_cc "${proto_name}.pb.cc")
    set(pb_h "${proto_name}.pb.h")

    list(APPEND PROTO_CC_FILES ${pb_cc})
endforeach()

add_custom_target(GenerateProtoFiles ALL DEPENDS ${PROTO_CC_FILES})

add_library(proto_cc OBJECT ${PROTO_CC_FILES})

add_dependencies(proto_cc GenerateProtoFiles)

set(PROTO_LIB ${PROTO_LIB} proto_cc PARENT_SCOPE)
