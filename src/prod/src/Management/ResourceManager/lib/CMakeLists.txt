set( LINUX_SOURCES
    ../Constants.cpp
    ../ReplicaActivityAsyncOperation.cpp
    ../AutoRetryAsyncOperation.cpp
    ../ResourceManagerAsyncOperation.cpp
    ../ClaimBasedAsyncOperation.cpp
    ../ClaimResourceAsyncOperation.cpp
    ../ReleaseResourceAsyncOperation.cpp
    ../Context.cpp
    ../ResourceDataItem.cpp
    ../ResourceManagerCore.cpp
    ../ResourceManagerService.cpp
)

include_directories("..")

add_library(objects_ResourceManager OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ResourceManager ../stdafx.h)

add_library(${lib_ResourceManager} STATIC $<TARGET_OBJECTS:objects_ResourceManager>)

set_target_properties(${lib_ResourceManager} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
