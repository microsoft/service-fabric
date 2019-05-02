set( LINUX_SOURCES
  ./dllmain.cpp
  ./stdafx.cpp
  ../../../../pal/wc16.c
)

include_directories(".")

add_library(objects_FabricUpgradeOrchestrationService OBJECT ${LINUX_SOURCES})

#add_precompiled_header(objects_FabricUpgradeOrchestrationService ./stdafx.h)

add_library(${lib_FabricUpgradeOrchestrationService} SHARED $<TARGET_OBJECTS:objects_FabricUpgradeOrchestrationService>)

set_target_properties(${lib_FabricUpgradeOrchestrationService} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set_target_properties(${lib_FabricUpgradeOrchestrationService} PROPERTIES 
    LINK_FLAGS -Wl,-Bsymbolic)
    
target_link_libraries(${lib_FabricUpgradeOrchestrationService}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeService}
  ${lib_Client}
  ${lib_ManagementCommon}
  ${lib_ManagementRepairManager}
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_ApiWrappers}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_LoadBalancing}
  ${lib_SystemServices}
  ${lib_ServiceGroup}
  ${lib_ImageModel}
  ${lib_ApiWrappers}
  ${lib_Query}
  ${lib_HealthManager}
  ${lib_LeaseAgent}
  ${lib_Federation}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_FabricRuntime}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  ssh2
  ssl
  crypto
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)
