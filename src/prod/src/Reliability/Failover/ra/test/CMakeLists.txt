include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FailoverRA.Test}
  # boost.test main
  ../../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../stdafx.cpp

  ../Test.Unit.ReadWriteStatusState.cpp
  ../Test.StateMachine.NodeUpAckProcessing.cpp
  ../Test.Unit.EntitySet.cpp
  ../Test.Unit.HealthReportReplicaTest.cpp
  ../Test.Unit.ReplicaUploadState.cpp
  ../Test.Unit.RetryableErrorState.cpp
  ../Test.Utility.EntityExecutionContextTestUtility.cpp
  ../Test.Utility.TestLog.cpp
  ../Test.Unit.FailoverManagerId.cpp
  ../Test.Unit.FailoverUnitPreCommitValidator.cpp
  ../Test.Unit.Infrastructure.FilteredVector.cpp
  ../Test.Unit.Infrastructure.MultipleEntityWorkManager.cpp
  ../Test.Unit.Infrastructure.LinearRetryPolicy.cpp
  ../Test.Unit.Infrastructure.RetryState.cpp
  ../Test.StateMachine.InMemoryCommits.cpp
  ../Test.StateMachine.MessageRetry.FMMessagePendingEntityList.cpp
  ../Test.Unit.MessageHandler.cpp
  ../Test.Functional.Infrastructure.BackgroundWorkManager.cpp
  ../Test.Functional.Infrastructure.BackgroundWorkManagerWithRetry.cpp
  ../Test.StateMachine.CancelUpgradeContext.cpp
  ../Test.Utility.FailoverUnitContainer.cpp
  ../Test.Unit.SetMembership.cpp
  ../Test.Utility.FederationWrapperStub.cpp
  ../Test.StateMachine.HostingAdapter.cpp
  ../Test.StateMachine.StateCleanup.cpp
  ../Test.StateMachine.MessageContext.cpp
  ../Test.StateMachine.MultipleReplicaCloseCompletionCheckAsyncOperation.cpp
  ../Test.StateMachine.MultipleReplicaCloseAsyncOperation.cpp
  ../Test.StateMachine.ReconfigurationSlowTraceEvent.cpp
  ../Test.Unit.NodeUpOperationFactory.cpp
  ../Test.Unit.ReconfigurationState.cpp
  ../Test.Unit.ReconfigurationHealthState.cpp
  ../Test.Unit.ReconfigurationStuckHealthReportDescriptorFactory.cpp
  ../Test.Utility.ReliabilitySubsystemStub.cpp
  ../Test.Utility.ConfigurationManager.cpp
  ../Test.StateMachine.ReplicaLifecycle.cpp
  ../Test.Unit.RetryableErrorStateThresholdCollection.cpp
  ../Test.Functional.RetryTimer.cpp
  ../Test.Utility.ScenarioTest.cpp
  ../Test.Unit.SendReceiveToFMOperation.cpp
  ../Test.Unit.ServiceTypeMapEntity.cpp
  ../Test.Utility.StateDefaults.cpp
  ../Test.Utility.StateItemHelper.cpp
  ../Test.StateMachine.DeleteReplica.cpp
  ../Test.StateMachine.EndpointPublish.cpp
  ../Test.StateMachine.GetLsn.cpp
  ../Test.StateMachine.GetLSNReply.cpp
  ../Test.StateMachine.ServiceTypeNotification.cpp
  ../Test.StateMachine.ServiceTypeEnabledDisabled.cpp
  ../Test.StateMachine.PartitionEnabledDisabled.cpp
  ../Test.StateMachine.cpp
  ../Test.StateMachine.UpdateserviceDescription.cpp
  ../Test.StateMachine.Query.cpp
  ../Test.Utility.StateReader.cpp
  ../Test.StateMachine.AddFailoverUnitChecks.cpp
  ../Test.Unit.Diagnostics.PerformanceData.cpp
  ../Test.Unit.Diagnostics.ReconfigurationPerformanceData.cpp
  ../Test.Functional.ReconfigurationStatistics.cpp
  ../Test.Unit.Infrastructure.EntityJobItem.cpp
  ../Test.Unit.Infrastructure.EntityMap.cpp
  ../Test.Unit.Infrastructure.EntityMapAlgorithm.cpp
  ../Test.Unit.Infrastructure.EntityRetryComponent.cpp
  ../Test.Unit.Infrastructure.EntityScheduler.cpp
  ../Test.Unit.Infrastructure.ExpiringMap.cpp
  ../Test.Unit.Infrastructure.LookupTable.cpp
  ../Test.Unit.Infrastructure.RAStopwatch.cpp
  ../Test.Functional.Storage.KeyValueStore.cpp
  ../Test.Utility.InfrastructureTestUtility.cpp
  ../Test.Unit.LocalFailoverUnitMap.cpp
  ../Test.Unit.Node.NodeDeactivationState.cpp
  ../Test.Unit.Node.ServiceTypeUpdateStalenessChecker.cpp
  ../Test.StateMachine.Node.NodeDeactivationStateProcessor.cpp
  ../Test.Unit.Node.ServiceTypeUpdatePendingLists.cpp
  ../Test.StateMachine.NodeActivationDeactivation.cpp
  ../Test.Unit.Proxy.OperationManager.cpp
  ../Test.Functional.PerformanceTest.cpp
  ../Test.Unit.Proxy.ConfigurationUtility.cpp
  ../Test.Unit.ServiceTypeRegistrationWrapper.cpp
  ../Test.Unit.ParallelPhase3Phase4.cpp
  ../Test.Unit.ReplicaStore.cpp
  ../Test.Unit.EndpointPublishState.cpp
  ../Test.StateMachine.DoReconfiguration.cpp
  ../Test.StateMachine.ReadWriteStatusRevokedNotification.cpp
  ../Test.StateMachine.Reconfiguration.cpp
  ../Test.StateMachine.ReconfigurationHealthReport.cpp
  ../Test.StateMachine.Close.cpp
  ../Test.StateMachine.ReplicaUpReply.cpp
  ../Test.StateMachine.FailoverUnitInitialization.cpp
  ../Test.Unit.FMMessageState.cpp
  ../Test.Utility.UnitTestContext.cpp
  ../Test.Unit.Upgrade.FabricCodeVersionClassifier.cpp
  ../Test.Unit.Upgrade.FabricUpgradeStalenessChecker.cpp
  ../Test.Utility.Upgrade.cpp
  ../Test.Unit.UpgradeMessageProcessor.cpp
  ../Test.Unit.UpgradeStateDescription.cpp
  ../Test.Unit.UpgradeStateMachine.cpp
  ../Test.Unit.Utility.SystemServiceHelper.cpp
  ../Test.Utility.cpp
  ../Test.StateMachine.ReplicaChangeRoleFailure.cpp
  ../Test.Unit.Node.PendingReplicaUploadState.cpp
  ../Test.Unit.Storage.Api.RowType.cpp
  ../Test.StateMachine.ResourceMonitorProcessing.cpp
)

#add_precompiled_header(${exe_FailoverRA.Test} ../stdafx.h)

set_target_properties(${exe_FailoverRA.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_FailoverRA.Test}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeService}
  ${lib_Testability}
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_Hosting2}
  ${lib_FabricTestCommon}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_Store}
  ${lib_TStore}
  ${lib_KtlLoggerNode}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_LoadBalancingCommon}
  ${lib_ServiceGroup}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Transport}
  ${lib_TestCommon}
  ${lib_FederationTestCommon}
  ${lib_Client}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_ManagementRepairManager}
  ${lib_FileStoreService}
  ${lib_ManagementCommon}
  ${lib_SystemServices}
  ${lib_ClientServerTransport}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${lib_NetworkInventoryManager}
  ${KtlUser}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricResources}
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

install(
    FILES ../RA.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunRATests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)

