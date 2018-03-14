set( LINUX_SOURCES
  ../ManagementConfig.cpp
)

include_directories("..")

add_library(objects_ManagementCommon OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ManagementCommon ../stdafx.h)
add_library(${lib_ManagementCommon} STATIC $<TARGET_OBJECTS:objects_ManagementCommon>)

set_target_properties(${lib_ManagementCommon} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

