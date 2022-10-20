// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingPublic.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"
#include "client/ClientConfig.h"
#include "Transport/TransportConfig.h"
#include "Transport/Throttle.h"

using namespace Api;
using namespace FabricTest;
using namespace Client;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ReliabilityTestApi;
using namespace ReliabilityTestApi::FailoverManagerComponentTestApi;
using namespace std;
using namespace Federation;
using namespace Common;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Naming;
using namespace Management;
using namespace Management::HealthManager;
using namespace FederationTestCommon;
using namespace TestCommon;
using namespace Hosting2;
using namespace HttpGateway;
using namespace Query;

extern bool IsTrue(wstring const & input);
extern bool IsFalse(wstring const & input);

ConfigurationSetter::ConfigurationSetter()
{
    PopulateConfigMap();
}

//Always use lower case strings for searching and inserting in the map
std::wstring ConfigurationSetter::GenerateMapKey(std::wstring const & key)
{
    std::wstring keyCopy = key;
    StringUtility::ToLower(keyCopy);
    return keyCopy;
}

//AddMapEntry operations for default entry keys and custom keys
void ConfigurationSetter::AddMapEntry(Common::ConfigEntryBase & entry)
{
    map_[GenerateMapKey(entry.Key)] = &entry;
}

void ConfigurationSetter::AddMapEntry(Common::ConfigEntryBase & entry, std::wstring customKey)
{
    map_[GenerateMapKey(customKey)] = &entry;
}

void ConfigurationSetter::PopulateConfigMap()
{
    #pragma region "ClientConfig"
    AddMapEntry(ClientConfig::GetConfig().HealthOperationTimeoutEntry);
    AddMapEntry(ClientConfig::GetConfig().HealthReportRetrySendIntervalEntry);
    AddMapEntry(ClientConfig::GetConfig().HealthReportSendBackOffStepInSecondsEntry);
    AddMapEntry(ClientConfig::GetConfig().HealthReportSendIntervalEntry);
    AddMapEntry(ClientConfig::GetConfig().HealthReportSendMaxBackOffIntervalEntry);
    AddMapEntry(ClientConfig::GetConfig().MaxNumberOfHealthReportsEntry);
    AddMapEntry(ClientConfig::GetConfig().MaxNumberOfHealthReportsPerMessageEntry);
    AddMapEntry(ClientConfig::GetConfig().MaxServiceChangePollBatchedRequestsEntry, L"NamingMaxServiceChangePollBatchedRequests");
    AddMapEntry(ClientConfig::GetConfig().ServiceChangePollIntervalEntry, L"NamingServiceChangePollInterval");
    #pragma endregion "ClientConfig"

    #pragma region "CommonConfig"
    AddMapEntry(CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluationEntry);
    AddMapEntry(CommonConfig::GetConfig().MaxNamingUriLengthEntry, L"NamingMaxNamingUriLength");
    AddMapEntry(CommonConfig::GetConfig().EnableReferenceTrackingEntry);
    AddMapEntry(CommonConfig::GetConfig().LeakDetectionTimeoutEntry);
    #pragma endregion "CommonConfig"

    #pragma region "FabricTestCommonConfig"
    AddMapEntry(FabricTestCommonConfig::GetConfig().StoreReplicationTimeoutEntry);
    AddMapEntry(FabricTestCommonConfig::GetConfig().IsBackupTestEntry);
    AddMapEntry(FabricTestCommonConfig::GetConfig().IsFalseProgressTestEntry);
    #pragma endregion "FabricTestCommonConfig"

    #pragma region "FabricTestSessionConfig"
    AddMapEntry(FabricTestSessionConfig::GetConfig().AllowHostFailureOnUpgradeEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().AllowServiceAndFULossOnRebuildEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().AllowUnexpectedFUsEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().AsyncNodeCloseEnabledEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().StartNodeCloseOnSeparateThreadEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().ChangeServiceLocationOnChangeRoleEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().DelayOpenAfterAbortNodeEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().NamingOperationRetryDelayEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().NamingOperationRetryTimeoutEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().NamingOperationTimeoutEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().NamingResolveRetryTimeoutEntry, L"NamingResolveRetryTimeout");
    AddMapEntry(FabricTestSessionConfig::GetConfig().OpenTimeoutEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().QueryOperationRetryDelayEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().QueryOperationRetryCountEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().ReportHealthThroughHMPrimaryEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().SkipDeleteVerifyForQuorumLostServicesEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().StoreClientTimeoutEntry);
    AddMapEntry(FabricTestSessionConfig::GetConfig().UseInternalHealthClientEntry);
    #pragma endregion "FabricTestSessionConfig"

    #pragma region "FailoverConfig"
    AddMapEntry(FailoverConfig::GetConfig().ApplicationUpgradeMaxReplicaCloseDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().BuildReplicaTimeLimitEntry);
    AddMapEntry(FailoverConfig::GetConfig().ChangeNotificationRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().ClusterPauseThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ClusterStableWaitDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().CreateInstanceTimeLimitEntry);
    AddMapEntry(FailoverConfig::GetConfig().DeletedFailoverUnitTombstoneDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().MinimumIntervalBetweenPeriodicStateCleanupEntry);
    AddMapEntry(FailoverConfig::GetConfig().DeletedReplicaKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().DroppedFailoverUnitTombstoneDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().DroppedReplicaKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ExpectedClusterSizeEntry);
    AddMapEntry(FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ExpectedReplicaUpgradeDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ExpectedNodeDeactivationDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().FMMessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().FTDetailedTraceIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().FabricUpgradeDownloadRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().FabricUpgradeMaxReplicaCloseDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().FabricUpgradeUpgradeRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().FabricUpgradeValidateRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().FullRebuildWaitDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().InBuildFailoverUnitKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().IsDeactivationInfoEnabledEntry);
    AddMapEntry(FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry);
    AddMapEntry(FailoverConfig::GetConfig().LocalHealthReportingTimerIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().LocalMessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().LocalMessageRetryTimerIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().MaxActionsPerIterationEntry);
    AddMapEntry(FailoverConfig::GetConfig().MaxNumberOfLoadReportsPerMessageEntry);
    AddMapEntry(FailoverConfig::GetConfig().MessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().MinActionRetryIntervalPerReplicaEntry);
    AddMapEntry(FailoverConfig::GetConfig().NodeDeactivationMaxReplicaCloseDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().NodeUpRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().OfflineReplicaKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().PerformSafetyChecksForNodeDeactivationIntentPauseEntry);
    AddMapEntry(FailoverConfig::GetConfig().PeriodicApiSlowTraceIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().PeriodicLoadPersistIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().PeriodicStateCleanupIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().PeriodicStateCleanupScanIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().PeriodicStateScanIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().PlacementConstraintsEntry, L"FMPlacementConstraints");
    AddMapEntry(FailoverConfig::GetConfig().PlacementTimeLimitEntry);
    AddMapEntry(FailoverConfig::GetConfig().ProcessingQueueSizeEntry);
    AddMapEntry(FailoverConfig::GetConfig().ProcessingQueueThreadCountEntry);
    AddMapEntry(FailoverConfig::GetConfig().ProxyOutgoingMessageRetryTimerIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().RAUpgradeProgressCheckIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().RAPMessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().RebuildPartitionTimeLimitEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReconfigurationHealthReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReconfigurationMessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReconfigurationTimeLimitEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReopenSuccessWaitIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaOpenFailureMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaOpenFailureWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaOpenFailureErrorReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaOpenFailureRestartThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaCloseFailureMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaCloseFailureWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaCloseFailureErrorReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaCloseFailureRestartThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaDeleteFailureMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaDeleteFailureWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaDeleteFailureErrorReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaDeleteFailureRestartThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaReopenFailureMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaReopenFailureWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaChangeRoleFailureRestartThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaChangeRoleFailureWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaChangeRoleFailureErrorReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaChangeRoleFailureMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ReplicaUpMessageRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().SendLoadReportIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().SendToFMRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().SendToFMTimeoutEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceApiHealthDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceLocationBroadcastIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceReconfigurationApiHealthDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationMaxRetryThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationRestartThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationErrorReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().SwapPrimaryRequestTimeoutEntry);
    AddMapEntry(FailoverConfig::GetConfig().UnknownNodeKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().RemovedNodeKeepDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().RemoteReplicaProgressQueryWaitDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationWarningReportThresholdAtDropEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationWarningReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().ServiceTypeRegistrationMaxRetryThresholdAtDropEntry);
    AddMapEntry(FailoverConfig::GetConfig().DropAllPLBMovementsEntry);
    AddMapEntry(FailoverConfig::GetConfig().FailoverUnitProxyCleanupIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().IsPreWriteStatusRevokeCatchupEnabledEntry);
    AddMapEntry(FailoverConfig::GetConfig().RestoreReplicaLocationAfterUpgradeEntry);
    AddMapEntry(FailoverConfig::GetConfig().SortUpgradeDomainNamesAsNumbersEntry);
    AddMapEntry(FailoverConfig::GetConfig().EnablePhase3Phase4InParallelEntry);
    AddMapEntry(FailoverConfig::GetConfig().GracefulReplicaShutdownMaxDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().GracefulReplicaCloseCompletionCheckIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().LazyPersistWaitDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().RecoverOnDataLossWaitDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().MaxRebuildRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().MinRebuildRetryIntervalEntry);
    AddMapEntry(FailoverConfig::GetConfig().RebuildHealthReportThresholdEntry);
    AddMapEntry(FailoverConfig::GetConfig().RAPApiOKHealthEventDurationEntry);
    AddMapEntry(FailoverConfig::GetConfig().StandByReplicaKeepDurationEntry, L"FMStandByReplicaKeepDuration");
    AddMapEntry(FailoverConfig::GetConfig().IsStrongSafetyCheckEnabledEntry);
    AddMapEntry(FailoverConfig::GetConfig().RemoveNodeOrDataUpReplicaTimeoutEntry);
    AddMapEntry(FailoverConfig::GetConfig().EnableConstraintCheckDuringFabricUpgradeEntry);
    AddMapEntry(FailoverConfig::GetConfig().RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckCompleteEntry);
    #pragma endregion "FailoverConfig"

    #pragma region "FederationConfig"
    AddMapEntry(FederationConfig::GetConfig().TokenAcquireTimeoutEntry);
    #pragma endregion "FederationConfig"

    #pragma region "HostingConfig"
    AddMapEntry(Hosting2::HostingConfig::GetConfig().ActivationMaxFailureCountEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().ActivationRetryBackoffIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().CacheCleanupScanIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().DeactivationGraceIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().DeactivationScanIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().DeactivationFailedRetryIntervalRangeEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().NTLMAuthenticationEnabledEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().NTLMAuthenticationPasswordSecretEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().RunAsPolicyEnabledEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().ServiceTypeDisableGraceIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().ServiceTypeRegistrationTimeoutEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().LocalResourceManagerTestModeEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().DisableContainersEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().ExclusiveModeDeactivationGraceIntervalEntry);
    AddMapEntry(Hosting2::HostingConfig::GetConfig().GovernOnlyMainMemoryForProcessesEntry);
    #pragma endregion "HostingConfig"

    #pragma region "ManagementConfig"
    AddMapEntry(ManagementConfig::GetConfig().RolloutFailureTimeoutEntry);
    AddMapEntry(ManagementConfig::GetConfig().ConsiderWarningAsErrorEntry);
    AddMapEntry(ManagementConfig::GetConfig().FabricUpgradeHealthCheckIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().FabricUpgradeStatusPollIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().HealthStoreCleanupGraceIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().HealthStoreCleanupIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().HealthStoreEntityWithoutSystemReportKeptIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().HealthStoreNodeEntityInvalidationDurationEntry);
    AddMapEntry(ManagementConfig::GetConfig().HealthStoreThrottleOperationCountEntry, L"HealthStoreThrottleOperationCount");
    AddMapEntry(ManagementConfig::GetConfig().InfrastructureTaskHealthCheckRetryTimeoutEntry);
    AddMapEntry(ManagementConfig::GetConfig().InfrastructureTaskHealthCheckStableDurationEntry);
    AddMapEntry(ManagementConfig::GetConfig().InfrastructureTaskHealthCheckWaitDurationEntry);
    AddMapEntry(ManagementConfig::GetConfig().InfrastructureTaskProcessingIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxCommunicationTimeoutEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxMessageSizeEntry, L"HMMaxMessageSize");
    AddMapEntry(ManagementConfig::GetConfig().MaxOperationTimeoutEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxExponentialOperationRetryDelayEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxPendingHealthCleanupJobCountEntry, L"MaxPendingHealthCleanupJobCount");
    AddMapEntry(ManagementConfig::GetConfig().MaxPendingHealthQueryCountEntry, L"MaxPendingHealthQueryCount");
    AddMapEntry(ManagementConfig::GetConfig().MaxPendingHealthReportCountEntry, L"MaxPendingHealthReportCount");
    AddMapEntry(ManagementConfig::GetConfig().EnableMaxPendingHealthReportSizeEstimationEntry, L"EnableMaxPendingHealthReportSizeEstimation");
    AddMapEntry(ManagementConfig::GetConfig().MaxPendingHealthReportSizeBytesEntry, L"MaxPendingHealthReportSizeBytes");
    AddMapEntry(ManagementConfig::GetConfig().MaxPercentUnhealthyApplicationsEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxPercentUnhealthyNodesEntry);
    AddMapEntry(ManagementConfig::GetConfig().MessageContentBufferRatioEntry, L"HMMessageContentBufferRatio");
    AddMapEntry(ManagementConfig::GetConfig().MinOperationTimeoutEntry);
    AddMapEntry(ManagementConfig::GetConfig().PlacementConstraintsEntry, L"CMPlacementConstraints");
    AddMapEntry(ManagementConfig::GetConfig().ReplicaSetCheckTimeoutRollbackOverrideEntry);
    AddMapEntry(ManagementConfig::GetConfig().UpgradeHealthCheckIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().EnableDefaultServicesUpgradeEntry);
    AddMapEntry(ManagementConfig::GetConfig().UpgradeStatusPollIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().EnableHealthCacheConsistencyCheckEntry);
    AddMapEntry(ManagementConfig::GetConfig().MinHealthCacheConsistencyCheckIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxHealthCacheConsistencyCheckIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxStatisticsDurationBeforeCachingEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxCacheStatisticsTimerIntervalEntry);
    AddMapEntry(ManagementConfig::GetConfig().StatisticsDurationOffsetEntry);
    AddMapEntry(ManagementConfig::GetConfig().ImageBuilderJobQueueThrottleEntry);
    AddMapEntry(ManagementConfig::GetConfig().ImageBuilderJobQueueDelayEntry);
    AddMapEntry(ManagementConfig::GetConfig().ImageBuilderUpgradeJobQueueThrottleEntry);
    AddMapEntry(ManagementConfig::GetConfig().ImageCachingEnabledEntry);
    AddMapEntry(ManagementConfig::GetConfig().NamingJobQueueThreadCountEntry);
    AddMapEntry(ManagementConfig::GetConfig().NamingJobQueueSizeEntry);
    AddMapEntry(ManagementConfig::GetConfig().NamingJobQueueMaxPendingWorkCountEntry);
    AddMapEntry(ManagementConfig::GetConfig().TestComposeDeploymentTestModeEntry);
    AddMapEntry(ManagementConfig::GetConfig().StandByReplicaKeepDurationEntry, L"CMStandByReplicaKeepDuration");
    AddMapEntry(ManagementConfig::GetConfig().MaxSuggestedNumberOfEntityHealthReportsEntry);
    AddMapEntry(ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransactionEntry);

    #pragma endregion "ManagementConfig"

    #pragma region "NamingConfig"
    AddMapEntry(NamingConfig::GetConfig().MaxNotificationReplyEntryCountEntry, L"NamingMaxNotificationReplyEntryCount");
    AddMapEntry(NamingConfig::GetConfig().PlacementConstraintsEntry, L"NamingPlacementConstraints");
    AddMapEntry(NamingConfig::GetConfig().RepairIntervalEntry);
    AddMapEntry(NamingConfig::GetConfig().RepairOperationTimeoutEntry);
    AddMapEntry(NamingConfig::GetConfig().ServiceDescriptionCacheLimitEntry, L"ServiceDescriptionCacheLimit");
    AddMapEntry(NamingConfig::GetConfig().NamingServiceHealthReportingTimerIntervalEntry, L"NamingServiceHealthReportingTimerInterval");
    AddMapEntry(NamingConfig::GetConfig().PrimaryRecoveryStartedHealthDurationEntry, L"PrimaryRecoveryStartedHealthDuration");
    AddMapEntry(NamingConfig::GetConfig().NamingServiceProcessOperationHealthDurationEntry, L"NamingServiceProcessOperationHealthDuration");
    AddMapEntry(NamingConfig::GetConfig().NamingServiceFailedOperationHealthGraceIntervalEntry, L"NamingServiceFailedOperationHealthGraceInterval");
    AddMapEntry(NamingConfig::GetConfig().NamingServiceFailedOperationHealthReportTimeToLiveEntry, L"NamingServiceFailedOperationHealthReportTimeToLive");
    AddMapEntry(NamingConfig::GetConfig().NamingServiceSlowOperationHealthReportTimeToLiveEntry, L"NamingServiceSlowOperationHealthReportTimeToLive");
    AddMapEntry(NamingConfig::GetConfig().MaxNamingServiceHealthReportsEntry, L"MaxNamingServiceHealthReports");
    AddMapEntry(NamingConfig::GetConfig().ServiceFailureTimeoutEntry, L"NamingServiceFailureTimeout");
    AddMapEntry(NamingConfig::GetConfig().RequestQueueThreadCountEntry);
    AddMapEntry(NamingConfig::GetConfig().RequestQueueSizeEntry);
    AddMapEntry(NamingConfig::GetConfig().MaxPendingRequestCountEntry);
    AddMapEntry(NamingConfig::GetConfig().StandByReplicaKeepDurationEntry, L"NamingStandByReplicaKeepDuration");
    #pragma endregion "NamingConfig"

    #pragma region "PLBConfig"
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().BalancingDelayAfterNodeDownEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().BalancingDelayAfterNewNodeEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintCheckEnabledEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintViolationReportingPolicyEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintViolationHealthReportLimitEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DetailedConstraintViolationHealthReportLimitEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DetailedVerboseHealthReportLimitEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().FastBalancingSearchTimeoutEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabledEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MinConstraintCheckIntervalEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MinLoadBalancingIntervalEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MinPlacementIntervalEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PLBActionRetryTimesEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PLBRewindIntervalEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PlaceChildWithoutParentEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PreferredLocationConstraintPriorityEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PreventIntermediateOvercommitEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ProcessPendingUpdatesIntervalEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ScoreImprovementThresholdEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UseMoveCostReportsEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UpgradeDomainConstraintPriorityEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().CheckAffinityForUpgradePlacementEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().AllowHigherChildTargetReplicaCountForAffinityEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().QuorumBasedReplicaDistributionPerUpgradeDomainsEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().QuorumBasedReplicaDistributionPerFaultDomainsEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().AllowConstraintCheckFixesDuringApplicationUpgradeEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaxPercentageToMoveForPlacementEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().AutoDetectAvailableResourcesEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UseSeparateSecondaryLoadEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().RelaxScaleoutConstraintDuringUpgradeEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().RelaxAffinityConstraintDuringUpgradeEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MoveParentToFixAffinityViolationEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MoveParentToFixAffinityViolationTransitionPercentageEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().SlowBalancingSearchTimeoutEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PreferNodesForContainerPlacementEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ThrottlingConstraintPriorityEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ThrottlePlacementPhaseEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ThrottleBalancingPhaseEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ThrottleConstraintCheckPhaseEntry);
    AddMapEntry(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PerNodeThrottlingCheckEntry);
    #pragma endregion "PLBConfig"

    #pragma region "RepairManagerConfig"
    AddMapEntry(Management::RepairManager::RepairManagerConfig::GetConfig().PlacementConstraintsEntry, L"RMPlacementConstraints");
    AddMapEntry(Management::RepairManager::RepairManagerConfig::GetConfig().EnableHealthChecksEntry);
    #pragma endregion "RepairManagerConfig"

    #pragma region "ServiceModelConfig"
    AddMapEntry(ServiceModelConfig::GetConfig().MaxMessageSizeEntry, L"NamingMaxMessageSize");
    AddMapEntry(ServiceModelConfig::GetConfig().MaxOperationTimeoutEntry, L"NamingMaxOperationTimeout");
    AddMapEntry(ServiceModelConfig::GetConfig().MaxPropertyNameLengthEntry, L"NamingMaxPropertyNameLength");
    AddMapEntry(ServiceModelConfig::GetConfig().MessageContentBufferRatioEntry, L"NamingMessageContentBufferRatio");
    AddMapEntry(ServiceModelConfig::GetConfig().QueryPagerContentRatioEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().QuorumLossWaitDurationEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().ServiceNotificationTimeoutEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().UserReplicaRestartWaitDurationEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().UserStandByReplicaKeepDurationEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDurationEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDurationEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().SystemMaxStandByReplicaCountEntry);
    AddMapEntry(ServiceModelConfig::GetConfig().UserMaxStandByReplicaCountEntry);
    #pragma endregion "ServiceModelConfig"

    #pragma region "StoreConfig"
    AddMapEntry(StoreConfig::GetConfig().EnableFileStreamFullCopyEntry);
    AddMapEntry(StoreConfig::GetConfig().EnableEndOfStreamAckOverrideEntry);
    AddMapEntry(StoreConfig::GetConfig().EseEnableBackgroundMaintenanceEntry);
    AddMapEntry(StoreConfig::GetConfig().EseEnableScanSerializationEntry);
    AddMapEntry(StoreConfig::GetConfig().EseScanIntervalMaxInSecondsEntry);
    AddMapEntry(StoreConfig::GetConfig().EseScanIntervalMinInSecondsEntry);
    AddMapEntry(StoreConfig::GetConfig().EseScanThrottleInMillesecondsEntry);
    AddMapEntry(StoreConfig::GetConfig().MaxFileStreamFullCopyWaitersEntry);
    AddMapEntry(StoreConfig::GetConfig().AssertOnOpenSharingViolationEntry);
    AddMapEntry(StoreConfig::GetConfig().AssertOnFatalErrorEntry);
    AddMapEntry(StoreConfig::GetConfig().TombstoneCleanupLimitEntry, L"RETombstoneCleanupLimit");
    AddMapEntry(StoreConfig::GetConfig().EnableSlowCommitTestEntry);
    AddMapEntry(StoreConfig::GetConfig().SlowCommitCountThresholdEntry);
    AddMapEntry(StoreConfig::GetConfig().EnableTombstoneCleanup2Entry);
    AddMapEntry(StoreConfig::GetConfig().EnableReferenceTrackingEntry, L"StoreEnableReferenceTracking");
    AddMapEntry(StoreConfig::GetConfig().TStoreInitializationRetryDelayEntry);
    AddMapEntry(StoreConfig::GetConfig().IgnoreOpenLocalStoreFlagEntry);
    #pragma endregion "StoreConfig"

    #pragma region "TransportConfig"
    AddMapEntry(Transport::TransportConfig::GetConfig().MemoryThrottleInMBEntry);
    AddMapEntry(Transport::TransportConfig::GetConfig().MemoryThrottleIntervalEntry);
    AddMapEntry(Transport::TransportConfig::GetConfig().UseUnreliableForRequestReplyEntry);
    #pragma endregion "TransportConfig"
}




//SetConfiguration overloads for each ConfigEntry type used in this file
void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<int> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(Int32_Parse(value));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<int64> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(Int64_Parse(value));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<unsigned int> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(Int32_Parse(value));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<double> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(Double_Parse(value));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<Common::TimeSpan> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(TimeSpan::FromSeconds(Double_Parse(value)));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<bool> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(IsTrue(value));
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<std::string> * configuration, std::wstring const & value)
{
    std::string newString;
    StringUtility::Utf16ToUtf8(value, newString);
    configuration->Test_SetValue(newString);
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<std::wstring> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(value);
}

void ConfigurationSetter::SetConfiguration(Common::ConfigEntry<Common::SecureString> * configuration, std::wstring const & value)
{
    configuration->Test_SetValue(SecureString(value));
}

//This method treats every case that is not a simple String -> Type conversion
bool ConfigurationSetter::ProcessCustomConfigurationSetter(StringCollection const & params)
{
    if (StringUtility::AreEqualCaseInsensitive(params[0], L"MaxApiDelayInterval"))
    {
        ApiFaultHelper::Get().SetMaxApiDelayInterval(Common::TimeSpan::FromSeconds(Double_Parse(params[1])));
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"MinApiDelayInterval"))
    {
        ApiFaultHelper::Get().SetMinApiDelayInterval(Common::TimeSpan::FromSeconds(Double_Parse(params[1])));
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"NamingMaxPropertySize"))
    {
        int size = Int32_Parse(params[1]);
        ServiceModel::Constants::Test_SetNamedPropertyMaxValueSize(size);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"HealthEntityCacheLoadStoreDataDelayInSeconds"))
    {
        double seconds = Double_Parse(params[1]);
        Management::HealthManager::Constants::Test_SetHealthEntityCacheLoadStoreDataDelayInSeconds(seconds);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"ReportHealthEntityApplicationType"))
    {
        bool reportApplicationType = IsTrue(params[1]);
        Management::ClusterManager::Constants::Test_SetReportHealthEntityApplicationType(reportApplicationType);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabledEntry.Key))
    {
        bool dummyPLBEnabled = IsTrue(params[1]);
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled = dummyPLBEnabled;
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled = !dummyPLBEnabled; // The difference between LoadBalancing Enabled and DummyPLB Enabled never seems to be used. We should consider whether we want to maintain this distinction (though in this case it doesn't seem to matter as LB is disabled at relevant times in fabrictestdispatcher.cpp)
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodeEntry.Section))
    {
        // Example: MaximumInBuildReplicasPerNode NodeType1 2 NodeType2 3
        StringMap mapThrottlingLimits;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapThrottlingLimits.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNode =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapThrottlingLimits);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodeBalancingThrottleEntry.Section))
    {
        // Example: MaximumInBuildReplicasPerNodeBalancingThrottle NodeType1 2 NodeType2 3
        StringMap mapThrottlingLimits;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapThrottlingLimits.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodeBalancingThrottle =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapThrottlingLimits);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodeConstraintCheckThrottleEntry.Section))
    {
        // Example: MaximumInBuildReplicasPerNodeConstraintCheckThrottle NodeType1 2 NodeType2 3
        StringMap mapThrottlingLimits;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapThrottlingLimits.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodeConstraintCheckThrottle =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapThrottlingLimits);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodePlacementThrottleEntry.Section))
    {
        // Example: MaximumInBuildReplicasPerNodePlacementThrottle NodeType1 2 NodeType2 3
        StringMap mapThrottlingLimits;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapThrottlingLimits.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MaximumInBuildReplicasPerNodePlacementThrottle =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapThrottlingLimits);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetricsEntry.Section))
    {
        // Example: DefragmentationMetrics CpuMin true Mem false
        StringMap mapDefragMetrics;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapDefragMetrics.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetrics =
        Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap::Parse(mapDefragMetrics);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricActivityThresholdsEntry.Section))
    {
        StringMap mapActivityThresholdsMetric;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapActivityThresholdsMetric.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricActivityThresholds =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapActivityThresholdsMetric);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricBalancingThresholdsEntry.Section))
    {
        StringMap mapBalancingThresholdsMetric;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapBalancingThresholdsMetric.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricBalancingThresholds =
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapBalancingThresholdsMetric);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricEmptyNodeThresholdsEntry.Section))
    {
        StringMap mapEmptyNodeMetric;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapEmptyNodeMetric.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricEmptyNodeThresholds =
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapEmptyNodeMetric);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().GlobalMetricWeightsEntry.Section))
    {
        StringMap mapGlobalWeightsMetric;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapGlobalWeightsMetric.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().GlobalMetricWeights =
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapGlobalWeightsMetric);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationEmptyNodeWeightEntry.Section))
    {
        StringMap mapDefragmentationEmptyNodeWeight;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapDefragmentationEmptyNodeWeight.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationEmptyNodeWeight =
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapDefragmentationEmptyNodeWeight);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationScopedAlgorithmEnabledEntry.Section))
    {
        StringMap mapDefragmentationScopedAlgorithmEnabled;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapDefragmentationScopedAlgorithmEnabled.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationScopedAlgorithmEnabled =
        Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap::Parse(mapDefragmentationScopedAlgorithmEnabled);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdEntry.Section))
    {
        StringMap mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold =
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().NodeBufferPercentageEntry.Section))
    {
        // Example: NodeBufferPercentage CpuMin 0.1 Mem 0.2
        StringMap mapNodeBuffPercentage;;
        for (int i = 1; i < params.size(); i += 2)
        {
            mapNodeBuffPercentage.insert(make_pair(params[i], params[i + 1]));
        }

        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().NodeBufferPercentage =
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapNodeBuffPercentage);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PLBHealthEventTTLEntry.Key))
    {
        bool healthEventsExpire = IsTrue(params[1]);
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PLBHealthEventTTL = healthEventsExpire ? Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PLBHealthEventTTL : Common::TimeSpan::FromSeconds(10000.0);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().FaultDomainConstraintPriorityEntry.Key))
    {
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().FaultDomainConstraintPriority = Int32_Parse(params[1]);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UpgradeDomainConstraintPriorityEntry.Key))
    {
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UpgradeDomainConstraintPriority = Int32_Parse(params[1]);
    }
    else
    {
        return false;
    }
    return true;
}

bool ConfigurationSetter::SetProperty(StringCollection const & params)
{
    if (params.size() > 0 && StringUtility::AreEqualCaseInsensitive(params[0], L"Defaults"))
    {
        FederationConfig::Test_Reset();
        return true;
    }

    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::SetPropertyCommand);
        return false;
    }

    std::map<std::wstring, ConfigEntryBase*>::iterator it;

    it = map_.find(GenerateMapKey(params[0]));
    //If the configuration entry is present in the map, process it as a default configuration behavior
    if (it != map_.end())
    {
        if (Common::ConfigEntry<int>* intEntry = dynamic_cast<Common::ConfigEntry<int> *>(it->second))
        {
            SetConfiguration(intEntry, params[1]);
        }
        else if (Common::ConfigEntry<unsigned int>* uintEntry = dynamic_cast<Common::ConfigEntry<unsigned int> *>(it->second))
        {
            SetConfiguration(uintEntry, params[1]);
        }
        else if (Common::ConfigEntry<int64>* int64Entry = dynamic_cast<Common::ConfigEntry<int64> *>(it->second))
        {
            SetConfiguration(int64Entry, params[1]);
        }
        else if (Common::ConfigEntry<double>* doubleEntry = dynamic_cast<Common::ConfigEntry<double> *>(it->second))
        {
            SetConfiguration(doubleEntry, params[1]);
        }
        else if (Common::ConfigEntry<bool>* boolEntry = dynamic_cast<Common::ConfigEntry<bool> *>(it->second))
        {
            SetConfiguration(boolEntry, params[1]);
        }
        else if (Common::ConfigEntry<std::string>* strEntry = dynamic_cast<Common::ConfigEntry<std::string> *>(it->second))
        {
            SetConfiguration(strEntry, params[1]);
        }
        else if (Common::ConfigEntry<std::wstring>* wstrEntry = dynamic_cast<Common::ConfigEntry<std::wstring> *>(it->second))
        {
            SetConfiguration(wstrEntry, params[1]);
        }
        else if (Common::ConfigEntry<Common::TimeSpan>* timeEntry = dynamic_cast<Common::ConfigEntry<Common::TimeSpan> *>(it->second))
        {
            SetConfiguration(timeEntry, params[1]);
        }
        else if (Common::ConfigEntry<Common::SecureString>* secureStrEntry = dynamic_cast<Common::ConfigEntry<Common::SecureString> *>(it->second))
        {
            SetConfiguration(secureStrEntry, params[1]);
        }
        else
        {
            return false;
        }
        return true;
    }
    //If its not, try to find a custom configuration behavior, if there isn't one, return false
    else
    {
        return ProcessCustomConfigurationSetter(params);
    }
}
