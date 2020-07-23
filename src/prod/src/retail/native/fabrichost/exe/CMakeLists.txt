include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FabricHost}
  ../Constants.cpp
  ../FabricActivatorService.cpp
  ../Main.cpp
  ../stdafx.cpp
  )

add_precompiled_header(${exe_FabricHost} ../stdafx.h)

set_target_properties(${exe_FabricHost} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 
    
target_link_libraries(${exe_FabricHost}
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
  ${lib_DnsServiceConfig}
  ${lib_Testability}
  ${lib_FileStoreService}
  ${lib_FabricTestCommon}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
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
  ${lib_Lease}
  ${lib_Federation}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_TestCommon}
  ${lib_FederationTestCommon}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${lib_NetworkInventoryManager}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  ssh2
  minizip
  z
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

