set( LINUX_SOURCES
    ./ServerWrapper.cpp
    ./stdafx.cpp
)

include_directories(".")

add_library(objects_AadWrapperServer OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_AadWrapperServer ./stdafx.h)
add_library(${lib_AadWrapperServer} STATIC $<TARGET_OBJECTS:objects_AadWrapperServer>)

set_target_properties(${lib_AadWrapperServer} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
