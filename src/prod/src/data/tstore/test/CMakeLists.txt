include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_TStore_Test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../Consolidation.Test.cpp
  ../BufferBufferConsolidation.Test
  ../BufferBufferStore.Test
  ../DiskMetdata.Test.cpp
  ../CheckpointFile.Test.cpp
  ../KeyValueListTest.cpp
  ../LongStringStore.Test.cpp
  ../MetadataFile.Test.cpp
  ../RecoveryStoreComponent.Test.cpp
  ../RecoveryApply.Test.cpp
  ../StoreCheckpoint.Test.cpp
  ../StoreOperationData.Test.cpp
  ../MockTransactionalReplicator.cpp
  ../PartitionSortedList.Test.cpp
  ../StringBufferConsolidation.Test.cpp
  ../StringBufferStore.Test.cpp
  ../Store.Test.1replica.cpp
  ../Store.Test.3replica.cpp
  ../Store.Test.Buffer.1replica.cpp
  ../Store.Test.Buffer.3replica.cpp
  ../VersionedItem.Test.cpp
  ../CheckpointFileComprehensive.Test.cpp
  ../StoreStateProviderFactory.cpp
  ../StoreStateProvider.Test.cpp
  ../MockStateManager.cpp
  ../Snapshot.Test.cpp
  ../ReadOnlySortedList.Test.cpp
  ../Enumeration.Test.cpp
  ../Enumeration.3Replica.Test.cpp
  ../SecondaryApply.Test.cpp
  ../UndoFalseProgress.Test.cpp
  ../TestTransactionContext.cpp
  ../MemoryBuffer.Test.cpp
  ../Store.Copy.Test.cpp
  ../Notification.Test.cpp
  ../KBufferComparer.cpp
  ../Store.Backup.Test.cpp
  ../MergeHelperTest.cpp
  ../Merge.Test.cpp
  ../Store.Test.Idempotency.cpp
  ../Store.Test.OperationsAllCombinations.cpp
  ../Store.Test.Timeout.cpp
  ../Store.MemorySize.Test.cpp
  ../StreamPool.Test.cpp
  ../Store.Sweep.Test.cpp
  ../SweepManager.Test.cpp
  ../Upgrade.Test.cpp
)

#add_precompiled_header(${exe_TStore_Test} ../stdafx.h)

set_target_properties(${exe_TStore_Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_TStore_Test}
  ${lib_TStore}
  ${lib_data_testcommon}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
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
    FILES ./tstore.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_TStore_Test}.cfg
)
