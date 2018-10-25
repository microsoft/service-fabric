set( LINUX_SOURCES
  ../ApplicationUpgradeProgressResult.cpp
  ../ChaosReportResult.cpp
  ../ChaosEventsSegmentResult.cpp
  ../CallSystemServiceResult.cpp
  ../ClientConfig.cpp
  ../ClientConnectionManager.cpp
  ../ClientHealthReporting.cpp
  ../ClientFactory.cpp
  ../constants.cpp
  ../FabricClientImpl.cpp
  ../FabricClientImpl.ClientAsyncOperationBase.cpp
  ../FabricClientImpl.CreateContainerAppAsyncOperation.cpp
  ../FabricClientImpl.LocationChangeNotificationAsyncOperation.cpp
  ../FabricClientImpl.RequestReplyAsyncOperation.cpp
  ../FabricClientImpl.ForwardToServiceAsyncOperation.cpp
  ../FabricClientImpl.DeleteServiceAsyncOperation.cpp
  ../FabricClientImpl.GetServiceDescriptionAsyncOperation.cpp
  ../FabricClientInternalSettings.cpp
  ../FabricClientInternalSettingsHolder.cpp
  ../FileReceiver.cpp
  ../FileSender.cpp
  ../FileTransferClient.cpp
  ../HealthReportClientState.cpp
  ../HealthReportingComponent.cpp
  ../IpcHealthReportingClient.cpp
  ../HealthReportingComponent.SequenceStream.cpp
  ../HealthReportSortedList.cpp
  ../HealthReportWrapper.cpp
  ../InvokeDataLossProgressResult.cpp
  ../InvokeQuorumLossProgressResult.cpp
  ../LruClientCacheEntry.cpp
  ../LruClientCacheManager.cpp
  ../LruClientCacheManager.CacheAsyncOperationBase.cpp
  ../LruClientCacheManager.GetPsdAsyncOperation.cpp
  ../LruClientCacheManager.ResolveServiceAsyncOperation.cpp
  ../LruClientCacheManager.UpdateCacheEntryAsyncOperation.cpp
  ../LruClientCacheManager.UpdateServiceTableEntryAsyncOperation.cpp
  ../LruClientCacheManager.ParallelUpdateCacheEntriesAsyncOperation.cpp
  ../LruClientCacheManager.PrefixResolveServiceAsyncOperation.cpp
  ../LruClientCacheManager.ProcessServiceNotificationAsyncOperation.cpp
  ../MovePrimaryResult.cpp
  ../MoveSecondaryResult.cpp
  ../NodeTransitionProgressResult.cpp
  ../OrchestrationUpgradeStatusResult.cpp
  ../UpgradeOrchestrationServiceStateResult.cpp
  ../PropertyBatchResult.cpp
  ../ResolvedServicePartitionCacheEntry.cpp
  ../ResolvedServicePartitionResult.cpp
  ../RestartDeployedCodePackageResult.cpp
  ../RestartNodeResult.cpp
  ../RestartPartitionProgressResult.cpp
  ../ServiceAddressTrackerManager.cpp
  ../ServiceAddressTracker.cpp
  ../ServiceAddressTrackerCallbacks.cpp
  ../ServiceAddressTrackerPollRequests.cpp
  ../ServiceNotificationClient.cpp
  ../ServiceNotificationResult.cpp
  ../StartNodeResult.cpp
  ../StopNodeResult.cpp
  ../UpgradeProgressResult.cpp
  ../Utility.cpp
  ../ChaosDescriptionResult.cpp
  ../ChaosScheduleDescriptionResult.cpp
)

include_directories("..")

add_library(objects_Client OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Client ../stdafx.h)
add_library(${lib_Client} STATIC $<TARGET_OBJECTS:objects_Client>)

set_target_properties(${lib_Client} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

