set( LINUX_SOURCES
  ../FabricNode.cpp
)

include_directories("..")

add_library(objects_FabricNode OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_FabricNode ../stdafx.h)
add_library(${lib_FabricNode} STATIC $<TARGET_OBJECTS:objects_FabricNode>)

set_target_properties(${lib_FabricNode} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

