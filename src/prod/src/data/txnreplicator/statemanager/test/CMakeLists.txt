include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_statemanager_test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../ApiDispatcher.Test.cpp
  ../BackCompatible.Copy.Test.cpp
  ../BackCompatible.Replication.Test.cpp
  ../Checkpoint.Test.cpp
  ../CheckpointFile.Test.cpp
  ../CheckpointFileBlocks.Test.cpp
  ../CheckpointFileProperties.Test.cpp
  ../CheckpointManager.Test.cpp
  ../Copy.Test.cpp
  ../CopyNamedOperationData.Test.cpp
  ../FaultStateProviderAPIs.Test.cpp
  ../ManagedOperationData.Test.cpp
  ../Metadata.Test.cpp
  ../MetadataManager.Test.cpp
  ../MetadataOperationData.Test.cpp
  ../MockLoggingReplicator.cpp
  ../NamedOperationData.Test.cpp  
  ../Notification.Test.cpp  
  ../Recovery.Test.cpp
  ../RedoOperationData.Test.cpp
  ../SerializableMetadata.Test.cpp
  ../StateManager.FunctionalTest.cpp
  ../StateManager.Test.cpp
  ../StateManager.HierarchyTest.cpp
  ../StateManagerLockContext.Test.cpp
  ../StateManagerTestBase.cpp
  ../StateManagerTransactionContext.Test.cpp
  ../TestCheckpointFileReadWrite.cpp
  ../TestHelper.cpp
  ../TestOperation.cpp
  ../TestLockContext.cpp
  ../TestOperationContext.cpp
  ../TestOperationDataStream.cpp
  ../TestReplicatedOperation.cpp
  ../TestStateManagerChangeHandler.cpp
  ../TestStateProvider.cpp
  ../TestStateProviderFactory.cpp
  ../TestStateProviderProperties.cpp
  ../TestTransaction.cpp
  ../TestTransactionManager.cpp
  ../Volatile.Test.cpp
)

#add_precompiled_header(${exe_statemanager_test} ../stdafx.h)

set_target_properties(${exe_statemanager_test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR} 
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_statemanager_test}
  ${lib_statemanager}
  ${lib_LoggingReplicator}
  ${lib_txnreplicator_common}
  ${lib_data_testcommon}
  ${lib_data_utilities}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_KComAdapter}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
  ssl
  crypto
  minizip
  z
  m
  rt
  pthread
  c
  dl
  xml2
  uuid
)

install(
    FILES ./statemanager.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_statemanager_test}.cfg
)

GET_FILENAME_COMPONENT(
    UPGRADE_FOLDER 
    ${EXTERNAL_DEPS_DIR}/Microsoft.ServiceFabric.Upgrade.Test.Internal
    REALPATH)

install(
    DIRECTORY ${UPGRADE_FOLDER}/
    DESTINATION ${TEST_OUTPUT_DIR}/data_upgrade
)
