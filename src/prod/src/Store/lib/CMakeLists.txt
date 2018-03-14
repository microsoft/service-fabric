set( LINUX_SOURCES
../AtomicOperation.cpp
../BackupRestoreData.cpp
../ComCopyContextEnumerator.cpp
../ComCopyOperationEnumerator.cpp
../ComReplicatedOperationData.cpp
../CopyType.cpp
../CopyStatistics.cpp
../Constants.cpp
../DeadlockDetector.cpp
../EseLocalStoreSettings.cpp
../StoreConfig.cpp
../StoreCounters.cpp

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
../KeyValueStoreReplica.cpp
../KeyValueStoreReplicaFactory.cpp
../KeyValueStoreTransaction.cpp
../KeyValueStoreQueryResultWrapper.cpp
../LocalStoreIncrementalBackupData.cpp
../OpenLocalStoreJobQueue.cpp
../PartialCopyProgressData.cpp
../PartitionedReplicaId.cpp
../ProgressVectorData.cpp
../ReplicaActivityId.cpp
../ReplicatedStoreEnumeratorBase.cpp
../ScopedActiveBackupState.cpp
../ReplicatedStore.BackupAsyncOperation.cpp
../ReplicatedStore.ChangeRoleAsyncOperation.cpp
../ReplicatedStore.cpp
../ReplicatedStore.Enumeration.cpp
../replicatedstoreevent.cpp
../ReplicatedStoreEventSource.cpp
../ReplicatedStore.NotificationManager.cpp
../ReplicatedStore.OnDataLossAsyncOperation.cpp
../ReplicatedStore.OpenAsyncOperation.cpp
../ReplicatedStore.RestoreAsyncOperation.cpp
../ReplicatedStore.SecondaryPump.cpp
../ReplicatedStoreSettings.cpp
../ReplicatedStore.SimpleTransaction.cpp
../ReplicatedStore.SimpleTransactionGroup.cpp
../replicatedstorestate.cpp
../ReplicatedStore.StateMachine.cpp
../ReplicatedStore.TransactionBase.cpp
../ReplicatedStore.Transaction.cpp
../ReplicatedStore.TransactionReplicator.cpp
../ReplicatedStore.TransactionReplicator.ThrottleState.cpp
../ReplicatedStore.TransactionReplicator.ThrottleStateMachine.cpp
../ReplicatedStore.TransactionTracker.cpp
../ReplicatedStore.HealthTracker.cpp
../ReplicatedStore.UpdateEpochAsyncOperation.cpp
../ReplicationOperationType.cpp
../SecondaryNotificationMode.cpp
../StoreBackupOption.cpp
../StoreData.cpp
../StoreFactory.cpp
../StoreTransactionTemplate.cpp
../TombstoneData.cpp
../TransactionSettings.cpp

../TSChangeHandler.cpp
../TSComponent.cpp
../TSEnumeration.cpp
../TSEnumerationBase.cpp
../TSTransaction.cpp
../TSLocalStore.cpp
../TSReplicatedStore.cpp
../TSReplicatedStoreSettings.cpp
../TSUnitTestStore.cpp
)

include_directories("..")

add_library(objects_store OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_store ../stdafx.h)
add_library(${lib_Store} STATIC $<TARGET_OBJECTS:objects_store>)


set_target_properties(${lib_Store} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
