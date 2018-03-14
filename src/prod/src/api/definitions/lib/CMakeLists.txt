set( LINUX_SOURCES
  ../ApiDefinitions.cpp
)

include_directories("..")

add_library(objects_ApiDefinitions OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ApiDefinitions ../stdafx.h)
add_library(${lib_ApiDefinitions} STATIC $<TARGET_OBJECTS:objects_ApiDefinitions>)

set_target_properties(${lib_ApiDefinitions} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

