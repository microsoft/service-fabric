include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FabricTestHost}
    ./TestHostMain.cpp
    ../ClientSecurityHelper.cpp
    ../ComCodePackageHost.cpp
    ../ComTestHostServiceFactory.cpp
    ../FabricTestHostDispatcher.cpp
    ../FabricTestHostSession.cpp
    ../TestHostServiceFactory.cpp
  )

set_target_properties(${exe_FabricTestHost} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_FabricTestHost}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_DnsServiceConfig}
  ${lib_UpgradeService}
  ${lib_Testability}
  ${lib_Hosting2}
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
  ${lib_Hosting2}
  ${lib_FileStoreService}
  ${lib_FabricTestCommon}
  ${lib_Store}
  ${lib_KtlLogger}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_LoadBalancing}
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
  ${lib_TestCommon}
  ${lib_TestHooks}
  ${lib_FederationTestCommon}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
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
  unwind
  unwind-x86_64
)

add_custom_command(TARGET ${exe_FabricTestHost} 
                   POST_BUILD 
                   COMMAND ${PROJECT_SOURCE_DIR}/linuxsetup/scripts/symsplit.sh ${TEST_OUTPUT_DIR} FabricTestHost.exe ${TEST_OUTPUT_DIR}/symbols)
