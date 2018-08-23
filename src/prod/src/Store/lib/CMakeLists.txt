set( LINUX_SOURCES
../AtomicOperation.cpp
../AzureStorageClient_Linux.cpp
../BackupRestoreData.cpp
../ComCopyContextEnumerator.cpp
../ComCopyOperationEnumerator.cpp
../ComReplicatedOperationData.cpp
../Constants.cpp
../CopyStatistics.cpp
../CopyType.cpp
../DeadlockDetector.cpp
../EseLocalStoreSettings.cpp
../FabricTimeController.cpp
../FabricTimeData.cpp
../FileStreamFullCopyContext.cpp
../FileStreamFullCopyManager.cpp
../FullCopyMode.cpp
../KeyValueStoreEnumeratorBase.cpp
../KeyValueStoreItemEnumerator.cpp
../KeyValueStoreItemMetadataEnumerator.cpp
../KeyValueStoreItemMetadataResult.cpp
../KeyValueStoreItemResult.cpp
../KeyValueStoreQueryResultWrapper.cpp
../KeyValueStoreReplica.cpp
../KeyValueStoreReplicaFactory.cpp
../KeyValueStoreTransaction.cpp
../LocalStoreIncrementalBackupData.cpp
../MigrationInitData.cpp
../OpenLocalStoreJobQueue.cpp
../PartialCopyProgressData.cpp
../PartitionedReplicaId.cpp
../ProgressVectorData.cpp
../ReplicaActivityId.cpp
../ReplicatedStore.BackupAsyncOperation.cpp
../ReplicatedStore.ChangeRoleAsyncOperation.cpp
../ReplicatedStore.cpp
../ReplicatedStore.Enumeration.cpp
../ReplicatedStore.HealthTracker.cpp
../ReplicatedStore.NotificationManager.cpp
../ReplicatedStore.OnDataLossAsyncOperation.cpp
../ReplicatedStore.OpenAsyncOperation.cpp
../ReplicatedStore.RestoreAsyncOperation.cpp
../ReplicatedStore.SecondaryPump.cpp
../ReplicatedStore.SimpleTransaction.cpp
../ReplicatedStore.SimpleTransactionGroup.cpp
../ReplicatedStore.StateMachine.cpp
../ReplicatedStore.Transaction.cpp
../ReplicatedStore.TransactionBase.cpp
../ReplicatedStore.TransactionReplicator.cpp
../ReplicatedStore.TransactionReplicator.ThrottleState.cpp
../ReplicatedStore.TransactionReplicator.ThrottleStateMachine.cpp
../ReplicatedStore.TransactionTracker.cpp
../ReplicatedStore.UpdateEpochAsyncOperation.cpp
../ReplicatedStoreEnumeratorBase.cpp
../replicatedstoreevent.cpp
../ReplicatedStoreEventSource.cpp
../ReplicatedStoreSettings.cpp
../replicatedstorestate.cpp
../ReplicationOperationType.cpp
../ScopedActiveBackupState.cpp
../SecondaryNotificationMode.cpp
../StoreBackupOption.cpp
../StoreConfig.cpp
../StoreCounters.cpp
../StoreData.cpp
../StoreFactory.cpp
../StoreTransactionTemplate.cpp
../TombstoneData.cpp
../TransactionSettings.cpp
../TSChangeHandler.cpp
../TSComponent.cpp
../TSEnumeration.cpp
../TSEnumerationBase.cpp
../TSLocalStore.cpp
../TSReplicatedStore.cpp
../TSReplicatedStoreSettings.cpp
../TSTransaction.cpp
../TSUnitTestStore.cpp
)

include_directories("..")

add_library(objects_store OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_store ../stdafx.h)
add_library(${lib_Store} STATIC $<TARGET_OBJECTS:objects_store>)


set_target_properties(${lib_Store} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
