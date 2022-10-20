include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FabricTest}
    ./testmain.cpp
    ../ApplicationBuilder.cpp
    ../CheckpointTruncationTimestampValidator.cpp
    ../ClientSecurityHelper.cpp
    ../ConfigurationSetter.cpp
    ../ComTestServiceFactory.cpp
    ../FabricTestDispatcher.cpp
    ../FabricTestDispatcher.Helper.cpp
    ../FabricTestSession.cpp
    ../FabricTestFederation.cpp
    ../FabricTestNode.cpp
    ../FabricTestQueryExecutor.cpp
    ../FabricTestQueryHandlers.cpp
    ../FabricTestRuntimeManager.cpp
    ../FabricTestApplicationMap.cpp
    ../FabricTestServiceMap.cpp
    ../FabricTestSessionConfig.cpp
    ../TestUpgradeFabricData.cpp
    ../FabricNodeWrapper.cpp
    ../FabricTestNamingState.cpp
    ../NativeImageStoreExecutor.cpp
    ../RandomFabricTestSession.cpp
    ../RandomSessionConfig.cpp
    ../RandomTestApplicationHelper.cpp
    ../SGOperationDataAsyncEnumerator.cpp
    ../SGUlonglongOperationData.cpp
    ../SGEmptyOperationData.cpp
    ../SGComStatefulService.cpp
    ../SGStatefulService.cpp
    ../SGComServiceFactory.cpp
    ../SGServiceFactory.cpp
    ../SGStatelessService.cpp
    ../SGComStatelessService.cpp
    ../SGComCompletedAsyncOperationContext.cpp
    ../ServiceLocationChangeCallback.cpp
    ../ServiceLocationChangeClientManager.cpp
    ../ServiceLocationChangeClient.cpp
    ../TestClientGateway.cpp
    ../TestFabricClientHelperClasses.cpp
    ../TestFabricClient.cpp
    ../TestFabricClientHealth.cpp
    ../TestFabricClientQuery.cpp
    ../TestFabricClientScenarios.cpp
    ../TestFabricClientSettings.cpp
    ../TestFabricClientUpgrade.cpp
    ../TestFabricUpgradeHostingImpl.cpp
    ../TestServiceFactory.cpp
    ../TestStoreClient.cpp
    ../TestStoreServiceProxy.cpp
    ../TestWatchDog.cpp
    ../TestHealthTable.cpp
    ../TestHostingSubsystem.cpp
    ../FabricTestClientsTracker.cpp
    ../FabricTestCommonConfig.cpp
    ../FabricTestConstants.cpp
    ../FabricTestCommands.cpp
    ../FabricTestCommandsTracker.cpp
    ../MessageType.cpp
    ../ServicePlacementData.cpp
    ../TestCodePackageContext.cpp
    ../TestMultiPackageHostContext.cpp
    ../TestServiceInfo.cpp
    ../ComAsyncOperationCallbackTestHelper.cpp
    ../ComTestPostBackupHandler.cpp
    ../ComCalculatorService.cpp
    ../CalculatorService.cpp
    ../ComTestStoreService.cpp
    ../ComTXRService.cpp
    ../ComTStoreService.cpp
    ../ComTestStateProvider2Factory.cpp
    ../TestStateProvider2.cpp
    ../ComTestPersistedStoreService.cpp
    ../ComTestReplicator.cpp
	../ComTestReplica.cpp
    ../ComTestReplicaMap.cpp
    ../BehaviorStore.cpp
    ../ApiFaultHelper.cpp
    ../TestPersistedStoreService.cpp
    ../TestStoreService.cpp
    ../TXRService.cpp
    ../TStoreService.cpp
    ../ServiceInitDataParser.cpp
    ../ReadWriteStatusValidator.cpp
  )

add_precompiled_header(${exe_FabricTest} ../stdafx.h)

set_target_properties(${exe_FabricTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition") 

target_link_libraries(${exe_FabricTest}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_KtlLoggerNode}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeOrchestrationService}
  ${lib_UpgradeService}
  ${lib_FaultAnalysisService}
  ${lib_BackupRestoreService}
  ${lib_ManagementCommon}
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${lib_ManagementRepairManager}
  ${Casablanca_LIBRARIES}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_Testability}
  ${lib_Hosting2}
  ${lib_Client}
  ${lib_FileStoreService}
  ${lib_FabricTestCommon}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TStore}
  ${lib_TransactionalReplicator}
  ${lib_txnreplicator_testcommon}
  ${lib_data_testcommon}
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
  ${lib_DnsServiceConfig}
  ${lib_GatewayResourceManager}
  ${KtlUser}
  ${KtlLoggerUser}
  ${lib_KtlLoggerShimUnify}
  ${KtlLoggerUser}
  ${Ktlfull}
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

install(
    FILES ${exe_FabricTest}.Linux.cfg 
    RENAME ${exe_FabricTest}.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES FailoverNDataRandom.Linux.params 
    RENAME FailoverNDataRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES UpgradeRandom.Linux.params 
    RENAME UpgradeRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES UpgradeNDataRandom.Linux.params 
    RENAME UpgradeNDataRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES RebuildNDataRandom.Linux.params 
    RENAME RebuildNDataRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES UnreliableNDataRandom.Linux.params 
    RENAME UnreliableNDataRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES RebuildRandom.Linux.params 
    RENAME RebuildRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES UnreliableRandom.Linux.params 
    RENAME UnreliableRandom.params 
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES
    FabricTestHelp.txt
    ClusterManifest-FabricUpgrade.xml
    docker-compose.yml
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    CODE "file( GLOB_RECURSE FabricTestFiles \"${CMAKE_CURRENT_SOURCE_DIR}/*.test\" \"${CMAKE_CURRENT_SOURCE_DIR}/*.Test\")"
    CODE "file( INSTALL \${FabricTestFiles} DESTINATION ${TEST_OUTPUT_DIR} )"
)

