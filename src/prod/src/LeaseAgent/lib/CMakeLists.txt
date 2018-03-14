set( LINUX_SOURCES
  ../DummyLeaseDriver.cpp
  ../LeaseAgent.cpp
  ../LeaseAgentEventSource.cpp
  ../LeaseConfig.cpp
  ../LeasePartner.cpp
)

include_directories("..")
include_directories("../pch")

add_library(objects_LeaseAgent OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_LeaseAgent ../stdafx.h)
add_library(${lib_LeaseAgent} STATIC $<TARGET_OBJECTS:objects_LeaseAgent>)

set_target_properties(${lib_LeaseAgent} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

