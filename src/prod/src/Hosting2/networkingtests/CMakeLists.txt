include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Hosting2NetworkingTest}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../OverlayIPAM.Test.cpp
  ../OverlayReservationManager.Test.cpp
  ../IPAddressProvider.Test.cpp
  ../ReservationManager.Test.cpp
  ../FlatIPConfiguration.Test.cpp
  ../IPAM.Test.cpp
  )

add_precompiled_header(${exe_Hosting2NetworkingTest} ../stdafx.h FORCEINCLUDE)

set_target_properties(${exe_Hosting2NetworkingTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Hosting2NetworkingTest}
  ${lib_Hosting2}
  ${lib_FabricNode}
  ${lib_Testability}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeOrchestrationService}
  ${lib_FaultAnalysisService}
  ${lib_BackupRestoreService}
  ${lib_ManagementCommon}
  ${lib_ImageStore}
  ${lib_ClusterManager}
  ${lib_Store}
  ${lib_TStore}
  ${lib_Communication}
  ${lib_KtlLogger}
  ${lib_Client}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_SystemServices}
  ${lib_Federation}
  ${lib_Hosting2}
  ${lib_Query}
  ${lib_ApiWrappers}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_LoadBalancing}
  ${lib_Replication}
  ${lib_HealthManager}
  ${lib_ImageStore}
  ${lib_FileStoreService}
  ${lib_UpgradeService}
  ${lib_ServiceModel}
  ${lib_ImageModel}
  ${lib_SystemServices}
  ${lib_StoreRepairPolicy}
  ${lib_FabricGateway}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${lib_ManagementRepairManager}
  ${Casablanca_LIBRARIES}
  ${lib_EntreeService}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Replication}
  ${lib_Common}
  ${lib_TransactionalReplicator}
  ${lib_ServiceGroup}
  ${lib_ApiWrappers}
  ${lib_AadWrapperServer}
  ${lib_FabricUUID}
  ${lib_InternalFabricUUID}
  ${lib_DnsServiceConfig}
  ${KtlUser}
  ${KtlLoggerUser}
  ${lib_KtlLoggerShimUPassthrough}
  ${KtlLoggerUser}
  ${Ktlfull}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
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

install(
    FILES ./RunHosting2NetworkingTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)