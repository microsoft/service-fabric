set( LINUX_SOURCES
    ../FabricTestCommonConfig.cpp
    ../FabricTestConstants.cpp
    ../FabricTestCommands.cpp
    ../MessageType.cpp
    ../ServicePlacementData.cpp
    ../TestCodePackageContext.cpp
    ../TestMultiPackageHostContext.cpp
    ../TestServiceInfo.cpp
    ../ComAsyncOperationCallbackTestHelper.cpp
    ../ComTestPostBackupHandler.cpp
    ../ComCalculatorService.cpp
    ../CalculatorService.cpp
    ../ComTestStoreService.cpp
    ../ComTestPersistedStoreService.cpp
    ../ReadWriteStatusValidator.cpp
    ../ComTestReplicator.cpp
	../ComTestReplica.cpp
    ../ComTestReplicaMap.cpp
    ../BehaviorStore.cpp
    ../ApiFaultHelper.cpp
    ../TestPersistedStoreService.cpp
    ../TestStoreService.cpp
    ../ServiceInitDataParser.cpp
   )

include_directories("..")

add_library(objects_FabricTestCommon OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_FabricTestCommon ../stdafx.h)
add_library(${lib_FabricTestCommon} STATIC $<TARGET_OBJECTS:objects_FabricTestCommon>)

set_target_properties(${lib_FabricTestCommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

