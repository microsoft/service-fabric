set( LINUX_SOURCES
    ../NoopStateProvider.cpp
    ../NoopStateProviderFactory.cpp
    ../TestAsyncOnDataLossContext.cpp
    ../TestBackupAsyncOperation.cpp
    ../TestBackupCallbackHandler.cpp
    ../TestComStateProviderFactory.cpp
    ../TestDataLossHandler.cpp
    ../TestOperationDataStream.cpp
    ../TestComProxyDataLossHandler.cpp
    ../TestLockContext.cpp
    ../TestRestoreAsyncOperation.cpp
    ../TestStateProvider.cpp
    ../TestStateProvider.UndoOperationData.cpp
    ../TestStateProviderFactory.cpp
    ../TestStateProviderProperties.cpp
    ../VersionedState.cpp
)

include_directories("..")

add_library(objects_txnreplicator_testcommon OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_txnreplicator_testcommon ../stdafx.h)

add_library(${lib_txnreplicator_testcommon} STATIC $<TARGET_OBJECTS:objects_txnreplicator_testcommon>)

set_target_properties(${lib_txnreplicator_testcommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

