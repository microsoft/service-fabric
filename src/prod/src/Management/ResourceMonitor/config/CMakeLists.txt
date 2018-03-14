set( LINUX_SOURCES
  ResourceMonitorServiceConfig.cpp
  stdafx.cpp
)

include_directories(".")

add_library(objects_ResourceMonitorConfig OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ResourceMonitorConfig stdafx.h FORCEINCLUDE)
add_library(${lib_ResourceMonitorConfig} STATIC $<TARGET_OBJECTS:objects_ResourceMonitorConfig>)
 
set_target_properties(${lib_ResourceMonitorConfig} PROPERTIES
     ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
