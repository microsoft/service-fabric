set (exe_tstorelonghaul "TStoreLonghaulService.exe" CACHE STRING "TStore longhaul test")

add_executable(${exe_tstorelonghaul}
  main.cpp
  TestComStateProvider2Factory.cpp
  Service.cpp
)

#add_precompiled_header(${exe_tstorelonghaul} stdafx.h)

target_link_libraries(${exe_tstorelonghaul}
    ${lib_TransactionalReplicator}
    ${lib_txnreplicator_testcommon}
    ${lib_data_utilities}
    ${lib_httptransport}
    ${lib_TestableService}
    ${lib_StatefulServiceBase}
    ${lib_FabricNode}
    ${lib_Communication}
    ${lib_ManagementSubsystem}
    ${lib_UpgradeService}
    ${lib_Testability}
    ${lib_ManagementCommon}
    ${lib_HealthManager}
    ${lib_ClusterManager}
    ${lib_EntreeService}
    ${lib_StoreService}
    ${lib_FabricGateway}
    ${lib_Hosting2}
    ${lib_FileStoreService}
    ${lib_Store}
    ${lib_ReliableConcurrentQueue}
    ${lib_TestHooks}
    ${lib_KtlLoggerNode}
    ${lib_TStore}
    ${lib_StoreRepairPolicy}
    ${lib_Replication}
    ${lib_TransactionalReplicator}
    ${lib_ApiWrappers}
    ${lib_LoadBalancing}
    ${lib_Failover}
    ${lib_ServiceGroup}
    ${lib_ImageModel}
    ${lib_Query}
    ${lib_Federation}
    ${lib_LeaseAgent}
    ${lib_Lease}
    ${lib_Transport}
    ${lib_Client}
    ${lib_ImageStore}
    ${lib_ManagementRepairManager}
    ${lib_SystemServices}
    ${lib_ClientServerTransport}
    ${lib_Common}
    ${lib_ServiceModel}
    ${Ktlfull}
    ${lib_FabricRuntime}
    ${lib_FabricCommon}
    ${lib_InternalFabricUUID}
    ${lib_FabricUUID}
    ssh2
    ssl
    crypto
    minizip
    z
    m
    rt
    pthread
    c
    dl
    xml2
    uuid
)

set_target_properties(${exe_tstorelonghaul} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
