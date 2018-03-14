set( LINUX_SOURCES
  ../InfrastructureServiceAgent.cpp
  ../InfrastructureServiceAgentFactory.cpp
  ../InfrastructureServiceMessage.cpp
)

include_directories("..")

add_library(objects_InfrastructureService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_InfrastructureService ../stdafx.h)

add_library(${lib_InfrastructureService} STATIC $<TARGET_OBJECTS:objects_InfrastructureService>)

set_target_properties(${lib_InfrastructureService} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
