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
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_Hosting2}
  ${lib_FabricTestCommon}
  ${lib_Store}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_LoadBalancing}
  ${lib_ServiceGroup}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Transport}
  ${lib_Client}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_ManagementRepairManager}
  ${lib_SystemServices}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_FileStoreService}
  ${lib_ManagementCommon}
  ${lib_ClientServerTransport}
  ${lib_TestCommon}
  ${lib_TestHooks}
  ${lib_FederationTestCommon}
  ${lib_Common}
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
)

add_custom_command(TARGET ${exe_FabricTestHost} 
                   POST_BUILD 
                   COMMAND ${PROJECT_SOURCE_DIR}/linuxsetup/scripts/symsplit.sh ${TEST_OUTPUT_DIR} FabricTestHost.exe ${TEST_OUTPUT_DIR}/symbols ${CMAKE_STRIP})
