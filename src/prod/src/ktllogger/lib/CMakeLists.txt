add_definitions(-DUPASSTHROUGH=1)

set( LINUX_SOURCES
    ../Constants.cpp
    ../KtlLoggerConfig.cpp
    ../KtlLoggerNode.cpp
    ../ktlloggerktl.cpp
)

include_directories("..")
include_directories("../sys/inc")
include_directories("../sys/ktlshim")
include_directories("../..")
include_directories("../pch")

add_library(objects_KtlLoggerNode OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_KtlLoggerNode ../stdafx.h)
add_library(${lib_KtlLoggerNode} STATIC $<TARGET_OBJECTS:objects_KtlLoggerNode>)

set_target_properties(${lib_KtlLoggerNode} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

