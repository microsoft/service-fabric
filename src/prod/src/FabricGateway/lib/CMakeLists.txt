set( LINUX_SOURCES
    ../FabricGateway.cpp
)

include_directories("..")
 

add_library(objects_FabricGateway OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_FabricGateway ../stdafx.h)
add_library(${lib_FabricGateway} STATIC $<TARGET_OBJECTS:objects_FabricGateway>)

set_target_properties(${lib_FabricGateway} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
