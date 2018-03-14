set( LINUX_SOURCES
  ../CancelRepairRequestAsyncOperation.cpp
  ../CleanupAsyncOperation.cpp
  ../ClientRequestAsyncOperation.cpp
  ../ClusterRepairResourceIdentifier.cpp
  ../Constants.cpp
  ../CreateRepairRequestAsyncOperation.cpp
  ../DeleteRepairRequestAsyncOperation.cpp
  ../ForceApproveRepairAsyncOperation.cpp
  ../HealthCheckAsyncOperation.cpp
  ../HealthCheckStoreData.cpp
  ../NodeImpact.cpp
  ../NodeImpactLevel.cpp
  ../NodeRepairImpactDescription.cpp
  ../NodeRepairTargetDescription.cpp
  ../ProcessRepairTaskContextAsyncOperation.cpp
  ../ProcessRepairTaskContextsAsyncOperation.cpp
  ../ProcessQueryAsyncOperation.cpp
  ../RepairImpactDescriptionBase.cpp
  ../RepairManagerConfig.cpp
  ../RepairManagerEventSource.cpp
  ../RepairManagerQueryMessageHandler.cpp
  ../RepairManagerServiceFactory.cpp
  ../RepairManagerServiceReplica.cpp
  ../RepairManagerState.cpp
  ../RepairPreparationAsyncOperation.cpp
  ../RepairRestorationAsyncOperation.cpp
  ../RepairResourceIdentifierBase.cpp
  ../RepairTargetDescriptionBase.cpp
  ../RepairTask.cpp
  ../RepairTaskContext.cpp
  ../RepairTaskHealthCheckHelper.cpp
  ../RepairTaskHistory.cpp
  ../UpdateRepairExecutionStateAsyncOperation.cpp
  ../UpdateRepairTaskHealthPolicyAsyncOperation.cpp
  ../UpdateRepairTaskHealthPolicyMessageBody.cpp
)

include_directories("..")

add_library(objects_ManagementRepairManager OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ManagementRepairManager ../stdafx.h)
add_library(${lib_ManagementRepairManager} STATIC $<TARGET_OBJECTS:objects_ManagementRepairManager>)

set_target_properties(${lib_ManagementRepairManager} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

