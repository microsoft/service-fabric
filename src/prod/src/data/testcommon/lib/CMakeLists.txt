set( LINUX_SOURCES
    ../ComAsyncOperationCallbackTestHelper.cpp
    ../TestComCodePackageActivationContext.cpp
    ../TestComStatefulServicePartition.cpp
    ../TestRuntimeFolders.cpp
    ../TestStatefulServicePartition.cpp
)

include_directories("..")

add_library(objects_data_testcommon OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_data_testcommon ../stdafx.h)

add_library(${lib_data_testcommon} STATIC $<TARGET_OBJECTS:objects_data_testcommon>)

set_target_properties(${lib_data_testcommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

