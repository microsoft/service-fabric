set( LINUX_SOURCES
    ../CommandLineParser.cpp
    ../TestCommonConfig.cpp
    ../TestConfigstore.cpp
    ../TestData.cpp
    ../PendingItemManager.cpp
    ../TestDispatcher.cpp
    ../TestSession.cpp
    ../TestVerifier.cpp
    ../ClientTestSession.cpp
    ../ServerTestSession.cpp
    ../DistributedSessionMessage.cpp
   )

include_directories("..")

add_library(objects_TestCommon OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_TestCommon ../stdafx.h)
add_library(${lib_TestCommon} STATIC $<TARGET_OBJECTS:objects_TestCommon>)

set_target_properties(${lib_TestCommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

