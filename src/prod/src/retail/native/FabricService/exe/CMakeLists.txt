include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Fabric}
  ../FabricServiceResource.cpp
  ../Main.cpp
  ../stdafx.cpp
  )

add_precompiled_header(${exe_Fabric} ../stdafx.h)

set_target_properties(${exe_Fabric} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 
    
target_link_libraries(${exe_Fabric}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeOrchestrationService}
  ${lib_UpgradeService}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_Client}
  ${lib_FaultAnalysisService}
  ${lib_BackupRestoreService}
  ${lib_ManagementCommon}
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_ImageStore}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${lib_ManagementRepairManager}
  ${lib_KtlLoggerNode}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_Hosting2}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_TestHooks}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_LoadBalancing}
  ${lib_FailoverCommon}
  ${lib_SystemServices}
  ${lib_ServiceGroup}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_HealthManager}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Federation}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_AadWrapperServer}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${lib_DnsServiceConfig}
  ${lib_GatewayResourceManager}
  ${KtlUser}
  ${KtlLoggerUser}
  ${lib_KtlLoggerShimUnify}
  ${KtlLoggerUser}
  ${Cxx}
  ${CxxABI}
  ssh2
  z
  ssl
  crypto
  minizip
  z
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)

