set( LINUX_SOURCES
)

include_directories("..")

add_library(objects_ReliabilityCommon OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliabilityCommon ../stdafx.h)

add_library(${lib_ReliabilityCommon} STATIC $<TARGET_OBJECTS:objects_ReliabilityCommon>)

set_target_properties(${lib_Common} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

