set( LINUX_SOURCES
  ../Constants.cpp
  ../CommunicationSubsystem.cpp
)

include_directories("..")

add_library(objects_Communication OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Communication ../stdafx.h)
add_library(${lib_Communication} STATIC $<TARGET_OBJECTS:objects_Communication>)

set_target_properties(${lib_Communication} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

