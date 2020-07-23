set( LINUX_SOURCES
    ../HealthReportFactory.cpp
    ../ApplicationInfo.cpp
    ../ApplicationUpgradeContext.cpp
    ../AutoScalingTask.cpp
    ../BackgroundManager.cpp
    ../BackgroundThreadContext.cpp
    ../CacheEntry.cpp
    ../ChangeNotificationMessageBody.cpp
    ../CommitJobQueue.cpp
    ../CommonJobQueue.cpp
    ../DeactivateNodesContext.cpp
    ../DeleteServiceAction.cpp
    ../DeleteServiceContext.cpp
    ../EventSource.FailoverUnit.cpp
    ../FabricUpgradeContext.cpp
    ../FabricUpgradeManager.cpp
    ../FailoverManager.cpp
    ../FailoverThrottle.cpp
    ../FailoverUnit.cpp
    ../FailoverUnitCache.cpp
    ../FailoverUnitCacheEntry.cpp
    ../FailoverUnitConfiguration.cpp
    ../FailoverUnitCountsContext.cpp
    ../FailoverUnitHealthState.cpp
    ../FailoverUnitJobQueue.cpp
    ../FailoverUnitMessageProcessor.cpp
    ../FailoverUnitMessageTask.cpp
    ../FauxLocalStore.cpp
    ../FMServiceLookupTable.cpp
    ../InBuildFailoverUnit.cpp
    ../InBuildFailoverUnitCache.cpp
    ../InstrumentedPLB.cpp
    ../LoadCache.cpp
    ../LoadInfo.cpp
    ../LockedFailoverUnitPtr.cpp
    ../MovementTask.cpp
    ../NodeCache.cpp
    ../NodeCache.PeriodicTaskCounts.cpp
    ../NodeCounts.cpp
    ../NodeDeactivationInfo.cpp
    ../NodeInfo.cpp
    ../PendingTask.cpp
    ../PerformanceCounters.cpp
    ../PlacementTask.cpp
    ../PlbApiCallName.cpp
    ../InstrumentedPLB.cpp
    ../FailoverUnitCountsContext.cpp
    ../QueueCounts.cpp
    ../QueryHelper.cpp
    ../QueueCounts.cpp
    ../RebuildContext.cpp
    ../ReconfigurationTask.cpp
    ../RepartitionInfo.cpp
    ../RepartitionType.cpp
    ../Replica.cpp
    ../ReplicaDownOperation.cpp
    ../ReplicaDownTask.cpp
    ../ReplicasUpdateOperation.cpp
    ../ReplicaUpdateTask.cpp
    ../SaveLookupVersionAction.cpp
    ../SendMessageAction.cpp
    ../Service.cpp
    ../ServiceCache.ContextCounts.cpp
    ../ServiceCache.cpp
    ../ServiceCache.CreateServiceAsyncOperation.cpp
    ../ServiceCache.DeleteServiceAsyncOperation.cpp
    ../ServiceCache.PeriodicTaskCounts.cpp
    ../ServiceCache.ProcessPLBSafetyCheckAsyncOperation.cpp
    ../ServiceCache.ProcessUpgradeAsyncOperation.cpp
    ../ServiceCache.RemoveFromDisabledNodesAsyncOperation.cpp
    ../ServiceCache.RepartitionServiceAsyncOperation.cpp
    ../ServiceCache.ServiceTypeNotificationAsyncOperation.cpp
    ../ServiceCache.UpdateServiceTypesAsyncOperation.cpp
    ../ServiceCounts.cpp
    ../ServiceFactory.cpp
    ../ServiceInfo.cpp
    ../ServiceLookupTable.cpp
    ../ServiceTableUpdateMessageBody.cpp
    ../ServiceToPartitionMapContext.cpp
    ../ServiceType.cpp
    ../StatelessCheckTask.cpp
    ../StateMachineAction.cpp
    ../StateMachineTask.cpp
    ../StateUpdateTask.cpp
    ../Store.cpp
    ../StoreData.cpp
    ../TimedRequestReceiverContext.cpp
    ../TraceAction.cpp
    ../TraceEnums.cpp
    ../UpdateServiceContext.cpp
    ../Upgrade.cpp
    ../UpgradeContext.cpp
    ../UpgradeDomains.cpp
    ../UpgradeDomainSortPolicy.cpp
    ../TestApi.FM.cpp
)

add_library(objects_FailoverFM OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_FailoverFM ../stdafx.h)

add_library(${lib_FailoverFM} STATIC $<TARGET_OBJECTS:objects_FailoverFM>)

set_target_properties(${lib_FailoverFM} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

