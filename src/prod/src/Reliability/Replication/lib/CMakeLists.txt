set(LINUX_SOURCES
../AckSender.cpp
../ApiMonitoringWrapper.cpp
../DecayAverage.cpp
../BatchedHealthReporter.cpp
../ComFromBytesOperation.cpp
../ComOperation.cpp
../ComOperationDataAsyncEnumerator.cpp
../ComOperationStream.cpp
../ComOperationStream.GetOperationOperation.cpp
../ComProxyAsyncEnumOperation.cpp
../ComProxyStatefulServicePartition.cpp
../ComProxyStateProvider.cpp
../ComReplicator.BaseOperation.cpp
../ComReplicator.BuildIdleReplicaOperation.cpp
../ComReplicator.CatchupReplicaSetOperation.cpp
../ComReplicator.ChangeRoleOperation.cpp
../ComReplicator.CloseOperation.cpp
../ComReplicator.cpp
../ComReplicatorFactory.cpp
../ComReplicator.OnDatalossOperation.cpp
../ComReplicator.OpenOperation.cpp
../ComReplicator.ReplicateOperation.cpp
../ComReplicator.UpdateEpochOperation.cpp
../ComUserDataOperation.cpp
../Constants.cpp
../CopyAsyncOperation.cpp
../CopyContextReceiver.cpp
../CopySender.cpp
../EnumOperators.cpp
../HealthReportType.cpp
../MustCatchupEnum.cpp
../OperationQueue.cpp
../OperationQueueEventSource.cpp
../OperationStream.cpp
../OperationStream.GetOperationAsyncOperation.cpp
../PrimaryReplicator.BuildIdleAsyncOperation.cpp
../PrimaryReplicator.CatchupAsyncOperation.cpp
../PrimaryReplicator.CloseAsyncOperation.cpp
../PrimaryReplicator.cpp
../PrimaryReplicator.ReplicateAsyncOperation.cpp
../PrimaryReplicator.UpdateEpochAsyncOperation.cpp
../REInternalSettings.cpp
../ReliableOperationSender.cpp
../RemoteSession.cpp
../RemoteSession.EstablishCopyAsyncOperation.cpp
../REPerformanceCounters.cpp
../ReplicaInformation.cpp
../ReplicaManager.cpp
../ReplicationDemuxer.cpp
../ReplicationEndpointId.cpp
../ReplicationQueueManager.cpp
../ReplicationSession.cpp
../ReplicationTransport.cpp
../Replicator.BuildIdleAsyncOperation.cpp
../Replicator.CatchupReplicaSetAsyncOperation.cpp
../Replicator.ChangeRoleAsyncOperation.cpp
../Replicator.CloseAsyncOperation.cpp
../Replicator.cpp
../ReplicatorEventSource.cpp
../Replicator.OnDatalossAsyncOperation.cpp
../Replicator.OpenAsyncOperation.cpp
../Replicator.ReplicateAsyncOperation.cpp
../ReplicatorState.cpp
../Replicator.UpdateEpochAsyncOperation.cpp
../SecondaryCopyReceiver.cpp
../SecondaryReplicationReceiver.cpp
../SecondaryReplicatorTraceHandler.cpp
../SecondaryReplicatorTraceInfo.cpp
../SecondaryReplicator.CloseAsyncOperation2.cpp
../SecondaryReplicator.CloseAsyncOperation.cpp
../SecondaryReplicator.cpp
../SecondaryReplicator.DrainQueueAsyncOperation.cpp
../SecondaryReplicator.UpdateEpochAsyncOperation.cpp
../standarddeviation.cpp
)

include_directories("..")
include_directories("../pch")

add_library(objects_replication OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_replication ../stdafx.h)
add_library(${lib_Replication} STATIC $<TARGET_OBJECTS:objects_replication>)


set_target_properties(${lib_Replication} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
