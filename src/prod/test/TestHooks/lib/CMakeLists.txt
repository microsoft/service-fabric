set( LINUX_SOURCES
    ../ApiSignalHelper.cpp
    ../EnvironmentVariables.cpp
    ../FaultInjector.cpp
    ../Signal.cpp
   )

include_directories("..")

add_library(objects_TestHooks OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_TestHooks ../stdafx.h)
add_library(${lib_TestHooks} STATIC $<TARGET_OBJECTS:objects_TestHooks>)

set_target_properties(${lib_TestHooks} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

