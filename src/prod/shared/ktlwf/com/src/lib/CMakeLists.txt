set( LINUX_SOURCES
  ../KComAdapter.cpp
  ../KComUtility.cpp
  ../AsyncCallInAdapter.cpp
  ../FabricAsyncContextBase.cpp
  ../FabricAsyncServiceBase.cpp
  ../AsyncCalloutAdapter.cpp
)

include_directories("..")
include_directories("../../inc")
include_directories("../pch")

add_library(objects_kcomadapter OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_kcomadapter ../stdafx.h)

add_library(${lib_KComAdapter} STATIC $<TARGET_OBJECTS:objects_kcomadapter>)

set_target_properties(${lib_KComAdapter} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
