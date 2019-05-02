include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FabricTypeHost}
  ../Main.cpp
  ../ComStatelessServiceFactory.cpp
  ../ComStatelessServiceInstance.cpp
  ../EndpointsDescription.cpp
  ../StatelessServiceTypeHost.cpp
  ../stdafx.cpp
  )

add_precompiled_header(${exe_FabricTypeHost} ../stdafx.h)

set_target_properties(${exe_FabricTypeHost} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}) 
    
target_link_libraries(${exe_FabricTypeHost}
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
  ${lib_Testability}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_TransactionalReplicator}
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
