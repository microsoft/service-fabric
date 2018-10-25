set( LINUX_SOURCES
    ../AttributesStoreData.cpp
    ../ApplicationAttributesStoreData.cpp
    ../ApplicationEntity.cpp
    ../ApplicationHealthId.cpp
    ../ApplicationsCache.cpp
    ../AutoCleanupEventsJobItem.cpp
    ../checkinmemoryentitydatajobitem.cpp
    ../CleanupEntityExpiredTransientEventsJobItem.cpp
    ../CleanupEntityJobItem.cpp
    ../cleanupentityjobitembase.cpp
    ../ClusterAttributesStoreData.cpp
    ../ClusterCache.cpp
    ../ClusterEntity.cpp
    ../Constants.cpp
    ../DeleteEntityJobItem.cpp
    ../DeployedApplicationAttributesStoreData.cpp
    ../DeployedApplicationEntity.cpp
    ../DeployedApplicationHealthId.cpp
    ../DeployedApplicationsCache.cpp
    ../DeployedServicePackageAttributesStoreData.cpp
    ../DeployedServicePackageEntity.cpp
    ../DeployedServicePackageHealthId.cpp
    ../DeployedServicePackagesCache.cpp
    ../EntityCacheManager.cpp
    ../EntityHealthEventStoreData.cpp
    ../EntityJobItem.cpp
    ../EntityJobItemState.cpp
    ../EntityJobQueueManager.cpp
    ../EntitySequenceStreamStoreData.cpp
    ../EntityStateFlags.cpp
    ../GetSequenceStreamProgressAsyncOperation.cpp
    ../HealthCount.cpp
    ../HealthEntity.cpp
    ../HealthEntityKind.cpp
    ../HealthEntityState.cpp
    ../HealthEntityCache.cpp
    ../HealthEntityChildren.cpp
    ../HealthEntityParent.cpp
    ../HealthEventStoreData.cpp
    ../HealthJobItemKind.cpp
    ../HealthManagerCommonEventSource.cpp
    ../HealthManagerEventSource.cpp
    ../HealthManagerMessage.cpp
    ../HealthManager.PerformanceCounters.cpp
    ../HealthManagerReplica.cpp
    ../IHealthJobItem.cpp
    ../NodeAttributesStoreData.cpp
    ../NodeEntity.cpp
    ../NodesCache.cpp
    ../PartitionAttributesStoreData.cpp
    ../PartitionsCache.cpp
    ../ProcessInnerQueryRequestAsyncOperation.cpp
    ../ProcessQueryRequestAsyncOperation.cpp
    ../ProcessRequestAsyncOperation.cpp    
    ../ProcessReportEntityJobItem.cpp
    ../ProcessReportRequestAsyncOperation.cpp
    ../ProcessSequenceStreamJobItem.cpp
    ../PartitionEntity.cpp
    ../QueryRequestContext.cpp    
    ../RequestContext.cpp
    ../RequestContextKind.cpp
    ../ReplicaAttributesStoreData.cpp
    ../ReplicaHealthId.cpp
    ../ReplicaEntity.cpp
    ../ReplicasCache.cpp
    ../ReplicatedStoreWrapper.cpp
    ../ReportRequestContext.cpp
    ../Resources.cpp
    ../SequenceStreamRequestContext.cpp
    ../SequenceStreamStoreData.cpp
    ../ServiceAttributesStoreData.cpp
    ../ServiceEntity.cpp
    ../ServicesCache.cpp
    ../ServiceHealthId.cpp
)

include_directories("..")

add_library(objects_HealthManager OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_HealthManager ../stdafx.h)

add_library(${lib_HealthManager} STATIC $<TARGET_OBJECTS:objects_HealthManager>)

set_target_properties(${lib_HealthManager} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

