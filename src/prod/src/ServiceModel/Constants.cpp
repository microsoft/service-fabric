// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace std;

namespace ServiceModel
{
    __int64 const Constants::UninitializedVersion(-1);

    Common::WStringLiteral const Constants::Name(L"Name");
    Common::WStringLiteral const Constants::DeploymentName(L"DeploymentName");
    Common::WStringLiteral const Constants::TypeName(L"TypeName");
    Common::WStringLiteral const Constants::HealthStateFilter(L"HealthStateFilter");
    Common::WStringLiteral const Constants::TypeVersion(L"TypeVersion");
    Common::WStringLiteral const Constants::ParameterList(L"ParameterList");
    Common::WStringLiteral const Constants::DefaultParameterList(L"DefaultParameterList");
    Common::WStringLiteral const Constants::ApplicationHealthPolicy(L"ApplicationHealthPolicy");
    Common::WStringLiteral const Constants::ApplicationHealthPolicyMap(L"ApplicationHealthPolicyMap");
    Common::WStringLiteral const Constants::ApplicationHealthPolicies(L"ApplicationHealthPolicies");
    Common::WStringLiteral const Constants::instances(L"instances");
    Common::WStringLiteral const Constants::instanceNames(L"instanceNames");
    Common::WStringLiteral const Constants::MaxPercentServicesUnhealthy(L"MaxPercentServicesUnhealthy");
    Common::WStringLiteral const Constants::MaxPercentDeployedApplicationsUnhealthy(L"MaxPercentDeployedApplicationsUnhealthy");
    Common::WStringLiteral const Constants::Value(L"Value");
    Common::WStringLiteral const Constants::Status(L"Status");
    Common::WStringLiteral const Constants::Parameters(L"Parameters");
    Common::WStringLiteral const Constants::HealthState(L"HealthState");
    Common::WStringLiteral const Constants::AggregatedHealthState(L"AggregatedHealthState");
    Common::WStringLiteral const Constants::PartitionStatus(L"PartitionStatus");
    Common::WStringLiteral const Constants::LastQuorumLossDurationInSeconds(L"LastQuorumLossDurationInSeconds");
    Common::WStringLiteral const Constants::ServiceManifestVersion(L"ServiceManifestVersion");
    Common::WStringLiteral const Constants::TargetApplicationTypeVersion(L"TargetApplicationTypeVersion");
    Common::WStringLiteral const Constants::TargetApplicationParameterList(L"TargetApplicationParameterList");
    Common::WStringLiteral const Constants::UpgradeKind(L"UpgradeKind");
    Common::WStringLiteral const Constants::UpgradeDescription(L"UpgradeDescription");
    Common::WStringLiteral const Constants::UpdateDescription(L"UpdateDescription");
    Common::WStringLiteral const Constants::RollingUpgradeMode(L"RollingUpgradeMode");
    Common::WStringLiteral const Constants::UpgradeReplicaSetCheckTimeoutInSeconds(L"UpgradeReplicaSetCheckTimeoutInSeconds");
    Common::WStringLiteral const Constants::ReplicaSetCheckTimeoutInMilliseconds(L"ReplicaSetCheckTimeoutInMilliseconds");
    Common::WStringLiteral const Constants::ForceRestart(L"ForceRestart");
    Common::WStringLiteral const Constants::MonitoringPolicy(L"MonitoringPolicy");
    Common::WStringLiteral const Constants::ClusterHealthPolicy(L"ClusterHealthPolicy");
    Common::WStringLiteral const Constants::EnableDeltaHealthEvaluation(L"EnableDeltaHealthEvaluation");
    Common::WStringLiteral const Constants::ClusterUpgradeHealthPolicy(L"ClusterUpgradeHealthPolicy");
    Common::WStringLiteral const Constants::MaxPercentNodesUnhealthyPerUpgradeDomain(L"MaxPercentNodesUnhealthyPerUpgradeDomain");
    Common::WStringLiteral const Constants::MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain(L"MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain");
    Common::WStringLiteral const Constants::DefaultApplicationHealthAggregationPolicy(L"DefaultApplicationHealthAggregationPolicy");
    Common::WStringLiteral const Constants::ApplicationHealthAggregationPolicyOverrides(L"ApplicationHealthAggregationPolicyOverrides");
    Common::WStringLiteral const Constants::MaxPercentNodesUnhealthy(L"MaxPercentNodesUnhealthy");
    Common::WStringLiteral const Constants::MaxPercentDeltaUnhealthyNodes(L"MaxPercentDeltaUnhealthyNodes");
    Common::WStringLiteral const Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes(L"MaxPercentUpgradeDomainDeltaUnhealthyNodes");
    Common::WStringLiteral const Constants::EntryPointLocation(L"EntryPointLocation");
    Common::WStringLiteral const Constants::ProcessId(L"ProcessId");
    Common::WStringLiteral const Constants::HostProcessId(L"HostProcessId");
    Common::WStringLiteral const Constants::LastExitCode(L"LastExitCode");
    Common::WStringLiteral const Constants::LastActivationTime(L"LastActivationTime");
    Common::WStringLiteral const Constants::LastExitTime(L"LastExitTime");
    Common::WStringLiteral const Constants::LastSuccessfulActivationTime(L"LastSuccessfulActivationTime");
    Common::WStringLiteral const Constants::LastSuccessfulExitTime(L"LastSuccessfulExitTime");
    Common::WStringLiteral const Constants::ActivationFailureCount(L"ActivationFailureCount");
    Common::WStringLiteral const Constants::ContinuousActivationFailureCount(L"ContinuousActivationFailureCount");
    Common::WStringLiteral const Constants::ExitFailureCount(L"ExitFailureCount");
    Common::WStringLiteral const Constants::ContinuousExitFailureCount(L"ContinuousExitFailureCount");
    Common::WStringLiteral const Constants::ActivationCount(L"ActivationCount");
    Common::WStringLiteral const Constants::ExitCount(L"ExitCount");
    Common::WStringLiteral const Constants::SetupEntryPoint(L"SetupEntryPoint");
    Common::WStringLiteral const Constants::MainEntryPoint(L"MainEntryPoint");
    Common::WStringLiteral const Constants::HasSetupEntryPoint(L"HasSetupEntryPoint");
    Common::WStringLiteral const Constants::ServiceKind(L"ServiceKind");
    Common::WStringLiteral const Constants::ReplicaId(L"ReplicaId");
    Common::WStringLiteral const Constants::ReplicaOrInstanceId(L"ReplicaOrInstanceId");
    Common::WStringLiteral const Constants::ReplicaRole(L"ReplicaRole");
    Common::WStringLiteral const Constants::PreviousConfigurationRole(L"PreviousConfigurationRole");
    Common::WStringLiteral const Constants::InstanceId(L"InstanceId");
    Common::WStringLiteral const Constants::ReplicaStatus(L"ReplicaStatus");
    Common::WStringLiteral const Constants::ReplicaState(L"ReplicaState");
    Common::WStringLiteral const Constants::LastInBuildDurationInSeconds(L"LastInBuildDurationInSeconds");
    Common::WStringLiteral const Constants::PartitionId(L"PartitionId");
    Common::WStringLiteral const Constants::ActivationId(L"ActivationId");
    Common::WStringLiteral const Constants::ServiceName(L"ServiceName");
    Common::WStringLiteral const Constants::Address(L"Address");
    Common::WStringLiteral const Constants::CodePackageName(L"CodePackageName");
    Common::WStringLiteral const Constants::ServiceTypeName(L"ServiceTypeName");
    Common::WStringLiteral const Constants::CodePackageInstanceId(L"CodePackageInstanceId");
    Common::WStringLiteral const Constants::ServiceGroupMemberDescription(L"ServiceGroupMemberDescription");
    Common::WStringLiteral const Constants::IsServiceGroup(L"IsServiceGroup");
    Common::WStringLiteral const Constants::CodeVersion(L"CodeVersion");
    Common::WStringLiteral const Constants::ConfigVersion(L"ConfigVersion");
    Common::WStringLiteral const Constants::Details(L"Details");
    Common::WStringLiteral const Constants::RunFrequencyInterval(L"RunFrequencyInterval");
    Common::WStringLiteral const Constants::ConsiderWarningAsError(L"ConsiderWarningAsError");
    Common::WStringLiteral const Constants::IgnoreExpiredEvents(L"IgnoreExpiredEvents");
    Common::WStringLiteral const Constants::ApplicationName(L"ApplicationName");
    Common::WStringLiteral const Constants::InitializationData(L"InitializationData");
    Common::WStringLiteral const Constants::PartitionDescription(L"PartitionDescription");
    Common::WStringLiteral const Constants::TargetReplicaSetSize(L"TargetReplicaSetSize");
    Common::WStringLiteral const Constants::MinReplicaSetSize(L"MinReplicaSetSize");
    Common::WStringLiteral const Constants::HasPersistedState(L"HasPersistedState");
    Common::WStringLiteral const Constants::InstanceCount(L"InstanceCount");
    Common::WStringLiteral const Constants::PlacementConstraints(L"PlacementConstraints");
    Common::WStringLiteral const Constants::CorrelationScheme(L"CorrelationScheme");
    Common::WStringLiteral const Constants::ServiceLoadMetrics(L"ServiceLoadMetrics");
    Common::WStringLiteral const Constants::DefaultMoveCost(L"DefaultMoveCost");
    Common::WStringLiteral const Constants::IsDefaultMoveCostSpecified(L"IsDefaultMoveCostSpecified");
    Common::WStringLiteral const Constants::ServicePackageActivationMode(L"ServicePackageActivationMode");
    Common::WStringLiteral const Constants::ServicePlacementPolicies(L"ServicePlacementPolicies");
    Common::WStringLiteral const Constants::Flags(L"Flags");
    Common::WStringLiteral const Constants::ReplicaRestartWaitDurationSeconds(L"ReplicaRestartWaitDurationSeconds");
    Common::WStringLiteral const Constants::QuorumLossWaitDurationSeconds(L"QuorumLossWaitDurationSeconds");
    Common::WStringLiteral const Constants::StandByReplicaKeepDurationSeconds(L"StandByReplicaKeepDurationSeconds");
    Common::WStringLiteral const Constants::ReplicaRestartWaitDurationInMilliseconds(L"ReplicaRestartWaitDurationInMilliseconds");
    Common::WStringLiteral const Constants::QuorumLossWaitDurationInMilliseconds(L"QuorumLossWaitDurationInMilliseconds");
    Common::WStringLiteral const Constants::StandByReplicaKeepDurationInMilliseconds(L"StandByReplicaKeepDurationInMilliseconds");
    Common::WStringLiteral const Constants::Id(L"Id");
    Common::WStringLiteral const Constants::IpAddressOrFQDN(L"IpAddressOrFQDN");
    Common::WStringLiteral const Constants::Type(L"Type");
    Common::WStringLiteral const Constants::Version(L"Version");
    Common::WStringLiteral const Constants::NodeStatus(L"NodeStatus");
    Common::WStringLiteral const Constants::NodeDeactivationIntent(L"NodeDeactivationIntent");
    Common::WStringLiteral const Constants::NodeDeactivationStatus(L"NodeDeactivationStatus");
    Common::WStringLiteral const Constants::NodeDeactivationTask(L"NodeDeactivationTask");
    Common::WStringLiteral const Constants::NodeDeactivationTaskId(L"NodeDeactivationTaskId");
    Common::WStringLiteral const Constants::NodeDeactivationTaskType(L"NodeDeactivationTaskType");
    Common::WStringLiteral const Constants::NodeDeactivationTaskIdPrefixRM(L"RM/");
    Common::WStringLiteral const Constants::NodeDeactivationTaskIdPrefixWindowsAzure(L"WindowsAzure");
    Common::WStringLiteral const Constants::NodeDeactivationInfo(L"NodeDeactivationInfo");
    Common::WStringLiteral const Constants::NodeUpTimeInSeconds(L"NodeUpTimeInSeconds");
    Common::WStringLiteral const Constants::NodeDownTimeInSeconds(L"NodeDownTimeInSeconds");
    Common::WStringLiteral const Constants::NodeUpAt(L"NodeUpAt");
    Common::WStringLiteral const Constants::NodeDownAt(L"NodeDownAt");
    Common::WStringLiteral const Constants::UpgradeDomain(L"UpgradeDomain");
    Common::WStringLiteral const Constants::FaultDomain(L"FaultDomain");
    Common::WStringLiteral const Constants::PartitionScheme(L"PartitionScheme");
    Common::WStringLiteral const Constants::Count(L"Count");
    Common::WStringLiteral const Constants::Names(L"Names");
    Common::WStringLiteral const Constants::LowKey(L"LowKey");
    Common::WStringLiteral const Constants::HighKey(L"HighKey");
    Common::WStringLiteral const Constants::MaxPercentReplicasUnhealthy(L"MaxPercentReplicasUnhealthy");
    Common::WStringLiteral const Constants::Scheme(L"Scheme");
    Common::WStringLiteral const Constants::Weight(L"Weight");
    Common::WStringLiteral const Constants::PrimaryDefaultLoad(L"PrimaryDefaultLoad");
    Common::WStringLiteral const Constants::SecondaryDefaultLoad(L"SecondaryDefaultLoad");
    Common::WStringLiteral const Constants::DefaultLoad(L"DefaultLoad");
    Common::WStringLiteral const Constants::FailureAction(L"FailureAction");
    Common::WStringLiteral const Constants::HealthCheckWaitDurationInMilliseconds(L"HealthCheckWaitDurationInMilliseconds");
    Common::WStringLiteral const Constants::HealthCheckStableDurationInMilliseconds(L"HealthCheckStableDurationInMilliseconds");
    Common::WStringLiteral const Constants::HealthCheckRetryTimeoutInMilliseconds(L"HealthCheckRetryTimeoutInMilliseconds");
    Common::WStringLiteral const Constants::UpgradeTimeoutInMilliseconds(L"UpgradeTimeoutInMilliseconds");
    Common::WStringLiteral const Constants::UpgradeDomainTimeoutInMilliseconds(L"UpgradeDomainTimeoutInMilliseconds");
    Common::WStringLiteral const Constants::UpgradeDuration(L"UpgradeDuration");
    Common::WStringLiteral const Constants::UpgradeDurationInMilliseconds(L"UpgradeDurationInMilliseconds");
    Common::WStringLiteral const Constants::CurrentUpgradeDomainDuration(L"CurrentUpgradeDomainDuration");
    Common::WStringLiteral const Constants::UpgradeDomainDurationInMilliseconds(L"UpgradeDomainDurationInMilliseconds");
    Common::WStringLiteral const Constants::MaxPercentPartitionsUnhealthy(L"MaxPercentPartitionsUnhealthy");
    Common::WStringLiteral const Constants::RepartitionDescription(L"RepartitionDescription");
    Common::WStringLiteral const Constants::PartitionKind(L"PartitionKind");
    Common::WStringLiteral const Constants::NamesToAdd(L"NamesToAdd");
    Common::WStringLiteral const Constants::NamesToRemove(L"NamesToRemove");
    Common::WStringLiteral const Constants::ServicePartitionKind(L"ServicePartitionKind");
    Common::WStringLiteral const Constants::PartitionInformation(L"PartitionInformation");
    Common::WStringLiteral const Constants::InQuorumLoss(L"InQuorumLoss");
    Common::WStringLiteral const Constants::DomainName(L"DomainName");
    Common::WStringLiteral const Constants::ManifestVersion(L"ManifestVersion");
    Common::WStringLiteral const Constants::NodeName(L"NodeName");
    Common::WStringLiteral const Constants::NodeNameCamelCase(L"nodeName");
    Common::WStringLiteral const Constants::NodeId(L"NodeId");
    Common::WStringLiteral const Constants::NodeInstanceId(L"NodeInstanceId");
    Common::WStringLiteral const Constants::Content(L"Content");
    WStringLiteral const Constants::HttpVerb(L"HttpVerb");
    GlobalWString const StringConstants::HttpVerb = make_global<wstring>(Constants::HttpVerb.cbegin());
    WStringLiteral const Constants::UriPath(L"UriPath");
    GlobalWString const StringConstants::UriPath = make_global<wstring>(Constants::UriPath.cbegin());
    WStringLiteral const Constants::HttpContentType(L"Content-Type");
    GlobalWString const StringConstants::HttpContentType = make_global<wstring>(Constants::HttpContentType.cbegin());
    WStringLiteral const Constants::HttpContentEncoding(L"Content-Encoding");
#ifdef PLATFORM_UNIX
    GlobalString const StringConstants::HttpContentEncoding = make_global<string>("Content-Encoding");
#else
    GlobalWString const StringConstants::HttpContentEncoding = make_global<wstring>(Constants::HttpContentEncoding.cbegin());
#endif
    WStringLiteral const Constants::HttpRequestBody(L"Body");
    GlobalWString const StringConstants::HttpRequestBody = make_global<wstring>(Constants::HttpRequestBody.cbegin());
    WStringLiteral const Constants::HttpResponseBody(L"Body");
    WStringLiteral const Constants::HttpStatus(L"Status");
    Common::WStringLiteral const Constants::CreateFabricDump(L"CreateFabricDump");
    Common::WStringLiteral const Constants::IsStateful(L"IsStateful");
    Common::WStringLiteral const Constants::UseImplicitHost(L"UseImplicitHost");
    Common::WStringLiteral const Constants::UseServiceFabricReplicatedStore(L"UseServiceFabricReplicatedStore");
    Common::WStringLiteral const Constants::UseImplicitFactory(L"UseImplicitFactory");
    Common::WStringLiteral const Constants::Extensions(L"Extensions");
    Common::WStringLiteral const Constants::LoadMetrics(L"LoadMetrics");
    Common::WStringLiteral const Constants::ServiceTypeDescription(L"ServiceTypeDescription");
    Common::WStringLiteral const Constants::ServiceGroupTypeDescription(L"ServiceGroupTypeDescription");
    Common::WStringLiteral const Constants::ServiceGroupTypeMemberDescription(L"ServiceGroupTypeMemberDescription");
    Common::WStringLiteral const Constants::IsSeedNode(L"IsSeedNode");
    Common::WStringLiteral const Constants::ReplicaRestartWaitDurationInMilliSeconds(L"ReplicaRestartWaitDurationInMilliSeconds");
    Common::WStringLiteral const Constants::RunAsUserName(L"RunAsUserName");
    Common::WStringLiteral const Constants::CodePackageEntryPointStatistics(L"CodePackageEntryPointStatistics");
    Common::WStringLiteral const Constants::Manifest(L"Manifest");
    Common::WStringLiteral const Constants::ServiceManifestName(L"ServiceManifestName");
    Common::WStringLiteral const Constants::ServicePackageActivationId(L"ServicePackageActivationId");
    Common::WStringLiteral const Constants::HostType(L"HostType");
    Common::WStringLiteral const Constants::HostIsolationMode(L"HostIsolationMode");
    Common::WStringLiteral const Constants::ApplicationTypeBuildPath(L"ApplicationTypeBuildPath");
    Common::WStringLiteral const Constants::Async(L"Async");
    Common::WStringLiteral const Constants::ApplicationPackageCleanupPolicy(L"ApplicationPackageCleanupPolicy");
    Common::WStringLiteral const Constants::ApplicationPackageDownloadUri(L"ApplicationPackageDownloadUri");
    Common::WStringLiteral const Constants::NextUpgradeDomain(L"NextUpgradeDomain");
    Common::WStringLiteral const Constants::State(L"State");
    Common::WStringLiteral const Constants::CodeFilePath(L"CodeFilePath");
    Common::WStringLiteral const Constants::ClusterManifestFilePath(L"ClusterManifestFilePath");
    Common::WStringLiteral const Constants::DeactivationIntent(L"DeactivationIntent");
    Common::WStringLiteral const Constants::ApplicationTypeVersion(L"ApplicationTypeVersion");
    Common::WStringLiteral const Constants::UpgradeDomains(L"UpgradeDomains");
    Common::WStringLiteral const Constants::UpgradeState(L"UpgradeState");
    Common::WStringLiteral const Constants::ProgressStatus(L"ProgressStatus");
    Common::WStringLiteral const Constants::UpgradeDomainName(L"UpgradeDomainName");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyServices(L"MaxPercentUnhealthyServices");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyPartitionsPerService(L"MaxPercentUnhealthyPartitionsPerService");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyReplicasPerPartition(L"MaxPercentUnhealthyReplicasPerPartition");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyDeployedApplications(L"MaxPercentUnhealthyDeployedApplications");
    Common::WStringLiteral const Constants::DefaultServiceTypeHealthPolicy(L"DefaultServiceTypeHealthPolicy");
    Common::WStringLiteral const Constants::ServiceTypeHealthPolicyMap(L"ServiceTypeHealthPolicyMap");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyNodes(L"MaxPercentUnhealthyNodes");
    Common::WStringLiteral const Constants::MaxPercentUnhealthyApplications(L"MaxPercentUnhealthyApplications");
    Common::WStringLiteral const Constants::TotalCount(L"TotalCount");
    Common::WStringLiteral const Constants::BaselineErrorCount(L"BaselineErrorCount");
    Common::WStringLiteral const Constants::BaselineTotalCount(L"BaselineTotalCount");
    Common::WStringLiteral const Constants::SourceId(L"SourceId");
    Common::WStringLiteral const Constants::Property(L"Property");
    Common::WStringLiteral const Constants::Description(L"Description");
    Common::WStringLiteral const Constants::SequenceNumber(L"SequenceNumber");
    Common::WStringLiteral const Constants::RemoveWhenExpired(L"RemoveWhenExpired");
    Common::WStringLiteral const Constants::TimeToLiveInMs(L"TimeToLiveInMilliSeconds");
    Common::WStringLiteral const Constants::SourceUtcTimestamp(L"SourceUtcTimestamp");
    Common::WStringLiteral const Constants::LastModifiedUtcTimestamp(L"LastModifiedUtcTimestamp");
    Common::WStringLiteral const Constants::LastOkTransitionAt(L"LastOkTransitionAt");
    Common::WStringLiteral const Constants::LastWarningTransitionAt(L"LastWarningTransitionAt");
    Common::WStringLiteral const Constants::LastErrorTransitionAt(L"LastErrorTransitionAt");
    Common::WStringLiteral const Constants::IsExpired(L"IsExpired");
    Common::WStringLiteral const Constants::HealthEvent(L"HealthEvent");
    Common::WStringLiteral const Constants::HealthEvents(L"HealthEvents");
    Common::WStringLiteral const Constants::OkCount(L"OkCount");
    Common::WStringLiteral const Constants::WarningCount(L"WarningCount");
    Common::WStringLiteral const Constants::ErrorCount(L"ErrorCount");
    Common::WStringLiteral const Constants::EntityKind(L"EntityKind");
    Common::WStringLiteral const Constants::HealthStateCount(L"HealthStateCount");
    Common::WStringLiteral const Constants::HealthStatistics(L"HealthStatistics");
    Common::WStringLiteral const Constants::HealthStateCountList(L"HealthStateCountList");
    Common::WStringLiteral const Constants::ExcludeHealthStatistics(L"ExcludeHealthStatistics");
    Common::WStringLiteral const Constants::IncludeSystemApplicationHealthStatistics(L"IncludeSystemApplicationHealthStatistics");
    Common::WStringLiteral const Constants::ServiceAggregatedHealthStates(L"ServiceHealthStates");
    Common::WStringLiteral const Constants::DeployedApplicationsAggregatedHealthStates(L"DeployedApplicationHealthStates");
    Common::WStringLiteral const Constants::PartitionAggregatedHealthState(L"PartitionHealthStates");
    Common::WStringLiteral const Constants::ReplicaAggregatedHealthState(L"ReplicaHealthStates");
    Common::WStringLiteral const Constants::DeployedServicePackageAggregatedHealthState(L"DeployedServicePackageHealthStates");
    Common::WStringLiteral const Constants::NextActivationTime(L"NextActivationTime");
    Common::WStringLiteral const Constants::NodeAggregatedHealthStates(L"NodeHealthStates");
    Common::WStringLiteral const Constants::ApplicationAggregatedHealthStates(L"ApplicationHealthStates");
    Common::WStringLiteral const Constants::SystemApplicationAggregatedHealthState(L"SystemApplicationAggregatedHealthState");
    Common::WStringLiteral const Constants::ServiceManifestNameFilter(L"ServiceManifestNameFilter");
    Common::WStringLiteral const Constants::ServicePackageActivationIdFilter(L"ServicePackageActivationIdFilter");
    Common::WStringLiteral const Constants::ApplicationNameFilter(L"ApplicationNameFilter");
    Common::WStringLiteral const Constants::ApplicationTypeNameFilter(L"ApplicationTypeNameFilter");
    Common::WStringLiteral const Constants::ApplicationDefinitionKind(L"ApplicationDefinitionKind");
    Common::WStringLiteral const Constants::ApplicationTypeDefinitionKind(L"ApplicationTypeDefinitionKind");
    Common::WStringLiteral const Constants::DeployedServicePackagesFilter(L"DeployedServicePackagesFilter");
    Common::WStringLiteral const Constants::NodeNameFilter(L"NodeNameFilter");
    Common::WStringLiteral const Constants::ReplicaOrInstanceIdFilter(L"ReplicaOrInstanceIdFilter");
    Common::WStringLiteral const Constants::PartitionIdFilter(L"PartitionIdFilter");
    Common::WStringLiteral const Constants::ReplicasFilter(L"ReplicasFilter");
    Common::WStringLiteral const Constants::PartitionsFilter(L"PartitionsFilter");
    Common::WStringLiteral const Constants::ServiceNameFilter(L"ServiceNameFilter");
    Common::WStringLiteral const Constants::DeployedApplicationsFilter(L"DeployedApplicationsFilter");
    Common::WStringLiteral const Constants::ServicesFilter(L"ServicesFilter");
    Common::WStringLiteral const Constants::ApplicationsFilter(L"ApplicationsFilter");
    Common::WStringLiteral const Constants::HealthPolicy(L"HealthPolicy");
    Common::WStringLiteral const Constants::NodesFilter(L"NodesFilter");
    Common::WStringLiteral const Constants::DeployedServicePackageFilters(L"DeployedServicePackageFilters");
    Common::WStringLiteral const Constants::ReplicaFilters(L"ReplicaFilters");
    Common::WStringLiteral const Constants::PartitionFilters(L"PartitionFilters");
    Common::WStringLiteral const Constants::DeployedApplicationFilters(L"DeployedApplicationFilters");
    Common::WStringLiteral const Constants::ServiceFilters(L"ServiceFilters");;
    Common::WStringLiteral const Constants::ApplicationFilters(L"ApplicationFilters");
    Common::WStringLiteral const Constants::NodeFilters(L"NodeFilters");
    Common::WStringLiteral const Constants::ReplicaHealthStateChunks(L"ReplicaHealthStateChunks");
    Common::WStringLiteral const Constants::PartitionHealthStateChunks(L"PartitionHealthStateChunks");
    Common::WStringLiteral const Constants::DeployedServicePackageHealthStateChunks(L"DeployedServicePackageHealthStateChunks");
    Common::WStringLiteral const Constants::DeployedApplicationHealthStateChunks(L"DeployedApplicationHealthStateChunks");
    Common::WStringLiteral const Constants::ServiceHealthStateChunks(L"ServiceHealthStateChunks");
    Common::WStringLiteral const Constants::ApplicationHealthStateChunks(L"ApplicationHealthStateChunks");
    Common::WStringLiteral const Constants::NodeHealthStateChunks(L"NodeHealthStateChunks");
    Common::WStringLiteral const Constants::Metadata(L"Metadata");
    Common::WStringLiteral const Constants::ServiceDnsName(L"ServiceDnsName");
    Common::WStringLiteral const Constants::ScalingPolicies(L"ScalingPolicies");
    Common::WStringLiteral const Constants::Key(L"Key");
    Common::WStringLiteral const Constants::LastBalancingStartTimeUtc(L"LastBalancingStartTimeUtc");
    Common::WStringLiteral const Constants::LastBalancingEndTimeUtc(L"LastBalancingEndTimeUtc");
    Common::WStringLiteral const Constants::LoadMetricInformation(L"LoadMetricInformation");
    Common::WStringLiteral const Constants::IsBalancedBefore(L"IsBalancedBefore");
    Common::WStringLiteral const Constants::IsBalancedAfter(L"IsBalancedAfter");
    Common::WStringLiteral const Constants::DeviationBefore(L"DeviationBefore");
    Common::WStringLiteral const Constants::DeviationAfter(L"DeviationAfter");
    Common::WStringLiteral const Constants::ActivityThreshold(L"ActivityThreshold");
    Common::WStringLiteral const Constants::BalancingThreshold(L"BalancingThreshold");
    Common::WStringLiteral const Constants::ClusterCapacity(L"ClusterCapacity");
    Common::WStringLiteral const Constants::ClusterLoad(L"ClusterLoad");
    Common::WStringLiteral const Constants::RemainingUnbufferedCapacity(L"RemainingUnbufferedCapacity");
    Common::WStringLiteral const Constants::NodeBufferPercentage(L"NodeBufferPercentage");
    Common::WStringLiteral const Constants::BufferedCapacity(L"BufferedCapacity");
    Common::WStringLiteral const Constants::RemainingBufferedCapacity(L"RemainingBufferedCapacity");
    Common::WStringLiteral const Constants::IsClusterCapacityViolation(L"IsClusterCapacityViolation");
    Common::WStringLiteral const Constants::NodeCapacity(L"NodeCapacity");
    Common::WStringLiteral const Constants::NodeRemainingCapacity(L"NodeRemainingCapacity");
    Common::WStringLiteral const Constants::NodeLoad(L"NodeLoad");
    Common::WStringLiteral const Constants::IsCapacityViolation(L"IsCapacityViolation");
    Common::WStringLiteral const Constants::NodeBufferedCapacity(L"NodeBufferedCapacity");
    Common::WStringLiteral const Constants::NodeRemainingBufferedCapacity(L"NodeRemainingBufferedCapacity");
    Common::WStringLiteral const Constants::CurrentNodeLoad(L"CurrentNodeLoad");
    Common::WStringLiteral const Constants::NodeCapacityRemaining(L"NodeCapacityRemaining");
    Common::WStringLiteral const Constants::BufferedNodeCapacityRemaining(L"BufferedNodeCapacityRemaining");
    Common::WStringLiteral const Constants::NodeLoadMetricInformation(L"NodeLoadMetricInformation");
    Common::WStringLiteral const Constants::Action(L"Action");
    Common::WStringLiteral const Constants::LoadMetricReports(L"LoadMetricReports");
    Common::WStringLiteral const Constants::PrimaryLoadMetricReports(L"PrimaryLoadMetricReports");
    Common::WStringLiteral const Constants::SecondaryLoadMetricReports(L"SecondaryLoadMetricReports");
    Common::WStringLiteral const Constants::CurrentConfigurationEpoch(L"CurrentConfigurationEpoch");
    Common::WStringLiteral const Constants::PrimaryEpoch(L"PrimaryEpoch");
    Common::WStringLiteral const Constants::ConfigurationVersion(L"ConfigurationVersion");
    Common::WStringLiteral const Constants::DataLossVersion(L"DataLossVersion");
    Common::WStringLiteral const Constants::ContinuationToken(L"ContinuationToken");
    Common::WStringLiteral const Constants::MaxResults(L"MaxResults");
    Common::WStringLiteral const Constants::Items(L"Items");
    Common::WStringLiteral const Constants::ApplicationTypeHealthPolicyMap(L"ApplicationTypeHealthPolicyMap");
    Common::WStringLiteral const Constants::ApplicationCapacity(L"ApplicationCapacity");
    Common::WStringLiteral const Constants::MinimumNodes(L"MinimumNodes");
    Common::WStringLiteral const Constants::MaximumNodes(L"MaximumNodes");
    Common::WStringLiteral const Constants::ApplicationMetrics(L"ApplicationMetrics");
    Common::WStringLiteral const Constants::ReservationCapacity(L"ReservationCapacity");
    Common::WStringLiteral const Constants::MaximumCapacity(L"MaximumCapacity");
    Common::WStringLiteral const Constants::TotalApplicationCapacity(L"TotalApplicationCapacity");
    Common::WStringLiteral const Constants::RemoveApplicationCapacity(L"RemoveApplicationCapacity");
    Common::WStringLiteral const Constants::ApplicationLoad(L"ApplicationLoad");
    Common::WStringLiteral const Constants::ApplicationLoadMetricInformation(L"ApplicationLoadMetricInformation");
    Common::WStringLiteral const Constants::NodeCount(L"NodeCount");
    Common::WStringLiteral const Constants::IsStopped(L"IsStopped");
    Common::WStringLiteral const Constants::IsConsistent(L"IsConsistent");
    Common::WStringLiteral const Constants::SubNames(L"SubNames");
    Common::WStringLiteral const Constants::PropertyName(L"PropertyName");
    Common::WStringLiteral const Constants::TypeId(L"TypeId");
    Common::WStringLiteral const Constants::CustomTypeId(L"CustomTypeId");
    Common::WStringLiteral const Constants::Parent(L"Parent");
    Common::WStringLiteral const Constants::SizeInBytes(L"SizeInBytes");
    Common::WStringLiteral const Constants::Properties(L"Properties");
    Common::WStringLiteral const Constants::Data(L"Data");
    Common::WStringLiteral const Constants::IncludeValue(L"IncludeValue");
    Common::WStringLiteral const Constants::Exists(L"Exists");
    Common::WStringLiteral const Constants::Operations(L"Operations");
    Common::WStringLiteral const Constants::ErrorMessage(L"ErrorMessage");
    Common::WStringLiteral const Constants::OperationIndex(L"OperationIndex");

    Common::WStringLiteral const Constants::QueryPagingDescription(L"QueryPagingDescription");

    Common::WStringLiteral const Constants::ChaosEvent(L"ChaosEvent");
    Common::WStringLiteral const Constants::ChaosEventsFilter(L"Filter");
    Common::WStringLiteral const Constants::ChaosParameters(L"ChaosParameters");
    Common::WStringLiteral const Constants::ClientType(L"ClientType");
    Common::WStringLiteral const Constants::Reason(L"Reason");
    Common::WStringLiteral const Constants::Faults(L"Faults");
    Common::WStringLiteral const Constants::MaxClusterStabilizationTimeoutInSeconds(L"MaxClusterStabilizationTimeoutInSeconds");
    Common::WStringLiteral const Constants::MaxConcurrentFaults(L"MaxConcurrentFaults");
    Common::WStringLiteral const Constants::WaitTimeBetweenIterationsInSeconds(L"WaitTimeBetweenIterationsInSeconds");
    Common::WStringLiteral const Constants::WaitTimeBetweenFaultsInSeconds(L"WaitTimeBetweenFaultsInSeconds");
    Common::WStringLiteral const Constants::TimeToRunInSeconds(L"TimeToRunInSeconds");
    Common::WStringLiteral const Constants::EnableMoveReplicaFaults(L"EnableMoveReplicaFaults");
    Common::WStringLiteral const Constants::Context(L"Context");
    Common::WStringLiteral const Constants::Map(L"Map");
    Common::WStringLiteral const Constants::TimeStampUtc(L"TimeStampUtc");
    Common::WStringLiteral const Constants::NodeTypeInclusionList(L"NodeTypeInclusionList");
    Common::WStringLiteral const Constants::ApplicationInclusionList(L"ApplicationInclusionList");
    Common::WStringLiteral const Constants::ChaosTargetFilter(L"ChaosTargetFilter");
    Common::WStringLiteral const Constants::Schedule(L"Schedule");
    Common::WStringLiteral const Constants::ChaosStatus(L"Status");
    Common::WStringLiteral const Constants::ChaosScheduleStatus(L"ScheduleStatus");
    Common::WStringLiteral const Constants::StartDate(L"StartDate");
    Common::WStringLiteral const Constants::ExpiryDate(L"ExpiryDate");
    Common::WStringLiteral const Constants::StartTime(L"StartTime");
    Common::WStringLiteral const Constants::EndTime(L"EndTime");
    Common::WStringLiteral const Constants::StartTimeUtc(L"StartTimeUtc");
    Common::WStringLiteral const Constants::EndTimeUtc(L"EndTimeUtc");
    Common::WStringLiteral const Constants::ChaosParametersMap(L"ChaosParametersDictionary");
    Common::WStringLiteral const Constants::Jobs(L"Jobs");
    Common::WStringLiteral const Constants::Days(L"Days");
    Common::WStringLiteral const Constants::Times(L"Times");
    Common::WStringLiteral const Constants::Hour(L"Hour");
    Common::WStringLiteral const Constants::Minute(L"Minute");
    Common::WStringLiteral const Constants::Sunday(L"Sunday");
    Common::WStringLiteral const Constants::Monday(L"Monday");
    Common::WStringLiteral const Constants::Tuesday(L"Tuesday");
    Common::WStringLiteral const Constants::Wednesday(L"Wednesday");
    Common::WStringLiteral const Constants::Thursday(L"Thursday");
    Common::WStringLiteral const Constants::Friday(L"Friday");
    Common::WStringLiteral const Constants::Saturday(L"Saturday");

    Common::WStringLiteral const Constants::ServiceStatus(L"ServiceStatus");
    Common::WStringLiteral const Constants::Kind(L"Kind");
    Common::WStringLiteral const Constants::KindLowerCase(L"kind");
    Common::WStringLiteral const Constants::UnhealthyEvent(L"UnhealthyEvent");
    Common::WStringLiteral const Constants::unhealthyEvaluation(L"unhealthyEvaluation");
    Common::WStringLiteral const Constants::UnhealthyEvaluations(L"UnhealthyEvaluations");
    Common::WStringLiteral const Constants::ApplicationUnhealthyEvaluations(L"ApplicationUnhealthyEvaluations");
    Common::WStringLiteral const Constants::HealthEvaluation(L"HealthEvaluation");
    Common::WStringLiteral const Constants::UpgradePhase(L"UpgradePhase");
    Common::WStringLiteral const Constants::SafetyCheckKind(L"SafetyCheckKind");
    Common::WStringLiteral const Constants::SafetyCheck(L"SafetyCheck");
    Common::WStringLiteral const Constants::PendingSafetyChecks(L"PendingSafetyChecks");
    Common::WStringLiteral const Constants::NodeUpgradeProgressList(L"NodeUpgradeProgressList");
    Common::WStringLiteral const Constants::CurrentUpgradeDomainProgress(L"CurrentUpgradeDomainProgress");
    Common::WStringLiteral const Constants::StartTimestampUtc(L"StartTimestampUtc");
    Common::WStringLiteral const Constants::FailureTimestampUtc(L"FailureTimestampUtc");
    Common::WStringLiteral const Constants::FailureReason(L"FailureReason");
    Common::WStringLiteral const Constants::UpgradeDomainProgressAtFailure(L"UpgradeDomainProgressAtFailure");
    Common::WStringLiteral const Constants::UpgradeStatusDetails(L"UpgradeStatusDetails");
    Common::WStringLiteral const Constants::ApplicationUpgradeStatusDetails(L"ApplicationUpgradeStatusDetails");
    Common::WStringLiteral const Constants::WorkDirectory(L"WorkDirectory");
    Common::WStringLiteral const Constants::LogDirectory(L"LogDirectory");
    Common::WStringLiteral const Constants::TempDirectory(L"TempDirectory");

    Common::WStringLiteral const Constants::MetricName(L"MetricName");
    Common::WStringLiteral const Constants::MaxInstanceCount(L"MaxInstanceCount");
    Common::WStringLiteral const Constants::MaxPartitionCount(L"MaxPartitionCount");
    Common::WStringLiteral const Constants::MinInstanceCount(L"MinInstanceCount");
    Common::WStringLiteral const Constants::MinPartitionCount(L"MinPartitionCount");
    Common::WStringLiteral const Constants::LowerLoadThreshold(L"LowerLoadThreshold");
    Common::WStringLiteral const Constants::UpperLoadThreshold(L"UpperLoadThreshold");
    Common::WStringLiteral const Constants::UseOnlyPrimaryLoad(L"UseOnlyPrimaryLoad");
    Common::WStringLiteral const Constants::ScaleIntervalInSeconds(L"ScaleIntervalInSeconds");
    Common::WStringLiteral const Constants::ScaleIncrement(L"ScaleIncrement");
    Common::WStringLiteral const Constants::ScalingTrigger(L"ScalingTrigger");
    Common::WStringLiteral const Constants::ScalingMechanism(L"ScalingMechanism");

    WStringLiteral const Constants::ReplicatorStatus(L"ReplicatorStatus");

    WStringLiteral const Constants::LastReplicationOperationReceivedTimeUtc(L"LastReplicationOperationReceivedTimeUtc");
    WStringLiteral const Constants::LastCopyOperationReceivedTimeUtc(L"LastCopyOperationReceivedTimeUtc");
    WStringLiteral const Constants::LastAcknowledgementSentTimeUtc(L"LastAcknowledgementSentTimeUtc");

    WStringLiteral const Constants::RemoteReplicators(L"RemoteReplicators");
    WStringLiteral const Constants::LastReceivedReplicationSequenceNumber(L"LastReceivedReplicationSequenceNumber");
    WStringLiteral const Constants::LastAppliedReplicationSequenceNumber(L"LastAppliedReplicationSequenceNumber");
    WStringLiteral const Constants::IsInBuild(L"IsInBuild");
    WStringLiteral const Constants::LastReceivedCopySequenceNumber(L"LastReceivedCopySequenceNumber");
    WStringLiteral const Constants::LastAppliedCopySequenceNumber(L"LastAppliedCopySequenceNumber");
    WStringLiteral const Constants::LastAcknowledgementProcessedTimeUtc(L"LastAcknowledgementProcessedTimeUtc");

    WStringLiteral const Constants::RemoteReplicatorAckStatus(L"RemoteReplicatorAcknowledgementStatus");
    WStringLiteral const Constants::AverageReceiveDuration(L"AverageReceiveDuration");
    WStringLiteral const Constants::AverageApplyDuration(L"AverageApplyDuration");
    WStringLiteral const Constants::NotReceivedCount(L"NotReceivedCount");
    WStringLiteral const Constants::ReceivedAndNotAppliedCount(L"ReceivedAndNotAppliedCount");
    WStringLiteral const Constants::CopyStreamAcknowledgementDetail(L"CopyStreamAcknowledgementDetail");
    WStringLiteral const Constants::ReplicationStreamAcknowledgementDetail(L"ReplicationStreamAcknowledgementDetail");

    WStringLiteral const Constants::ReplicationQueueStatus(L"ReplicationQueueStatus");
    WStringLiteral const Constants::CopyQueueStatus(L"CopyQueueStatus");
    WStringLiteral const Constants::QueueUtilizationPercentage(L"QueueUtilizationPercentage");
    WStringLiteral const Constants::FirstSequenceNumber(L"FirstSequenceNumber");
    WStringLiteral const Constants::CompletedSequenceNumber(L"CompletedSequenceNumber");
    WStringLiteral const Constants::CommittedSequenceNumber(L"CommittedSequenceNumber");
    WStringLiteral const Constants::LastSequenceNumber(L"LastSequenceNumber");
    WStringLiteral const Constants::QueueMemorySize(L"QueueMemorySize");

    WStringLiteral const Constants::CurrentValue(L"CurrentValue");
    WStringLiteral const Constants::LastReportedUtc(L"LastReportedUtc");
    WStringLiteral const Constants::CurrentServiceOperation(L"CurrentServiceOperation");
    WStringLiteral const Constants::CurrentServiceOperationStartTimeUtc(L"CurrentServiceOperationStartTimeUtc");
    WStringLiteral const Constants::ReportedLoad(L"ReportedLoad");
    WStringLiteral const Constants::CurrentReplicatorOperation(L"CurrentReplicatorOperation");
    WStringLiteral const Constants::CurrentReplicatorOperationStartTimeUtc(L"CurrentReplicatorOperationStartTimeUtc");
    WStringLiteral const Constants::ReadStatus(L"ReadStatus");
    WStringLiteral const Constants::WriteStatus(L"WriteStatus");
    WStringLiteral const Constants::ServicePackageInstanceId(L"ServicePackageInstanceId");
    WStringLiteral const Constants::ReplicaInstanceId(L"ReplicaInstanceId");

    GlobalWString Constants::InfrastructureServicePrimaryCountName = make_global<std::wstring>(L"__InfrastructureServicePrimaryCount__");
    GlobalWString Constants::InfrastructureServiceReplicaCountName = make_global<std::wstring>(L"__InfrastructureServiceReplicaCount__");

    GlobalWString const Constants::ValidateTokenAction = make_global<std::wstring>(L"ValidateToken");

    GlobalWString Constants::EventSystemSourcePrefix = make_global<std::wstring>(L"System");

    WStringLiteral const Constants::Port(L"Port");
    WStringLiteral const Constants::ClusterConnectionPort(L"ClusterConnectionPort");

    WStringLiteral const Constants::ApplicationIds(L"ApplicationIds");
    WStringLiteral const Constants::ApplicationTypeName(L"ApplicationTypeName");
    WStringLiteral const Constants::CodePackageIds(L"CodePackageIds");
    WStringLiteral const Constants::ConfigPackageIds(L"ConfigPackageIds");
    WStringLiteral const Constants::DataPackageIds(L"DataPackageIds");

    WStringLiteral const Constants::PackageSharingScope(L"PackageSharingScope");
    WStringLiteral const Constants::SharedPackageName(L"SharedPackageName");
    WStringLiteral const Constants::PackageSharingPolicy(L"PackageSharingPolicy");
    WStringLiteral const Constants::FMVersion(L"FMVersion");
    WStringLiteral const Constants::StoreVersion(L"StoreVersion");
    WStringLiteral const Constants::GenerationNumber(L"GenerationNumber");
    WStringLiteral const Constants::Generation(L"Generation");
    WStringLiteral const Constants::Endpoints(L"Endpoints");
    WStringLiteral const Constants::PreviousRspVersion(L"PreviousRspVersion");

    Common::WStringLiteral const Constants::Scope(L"Scope");
    Common::WStringLiteral const Constants::TaskId(L"TaskId");
    Common::WStringLiteral const Constants::Target(L"Target");
    Common::WStringLiteral const Constants::Executor(L"Executor");
    Common::WStringLiteral const Constants::ExecutorData(L"ExecutorData");
    Common::WStringLiteral const Constants::ResultStatus(L"ResultStatus");
    Common::WStringLiteral const Constants::ResultCode(L"ResultCode");
    Common::WStringLiteral const Constants::ResultDetails(L"ResultDetails");
    Common::WStringLiteral const Constants::History(L"History");
    Common::WStringLiteral const Constants::CreatedUtcTimestamp(L"CreatedUtcTimestamp");
    Common::WStringLiteral const Constants::ClaimedUtcTimestamp(L"ClaimedUtcTimestamp");
    Common::WStringLiteral const Constants::PreparingUtcTimestamp(L"PreparingUtcTimestamp");
    Common::WStringLiteral const Constants::ApprovedUtcTimestamp(L"ApprovedUtcTimestamp");
    Common::WStringLiteral const Constants::ExecutingUtcTimestamp(L"ExecutingUtcTimestamp");
    Common::WStringLiteral const Constants::RestoringUtcTimestamp(L"RestoringUtcTimestamp");
    Common::WStringLiteral const Constants::CompletedUtcTimestamp(L"CompletedUtcTimestamp");
    Common::WStringLiteral const Constants::PreparingHealthCheckStartUtcTimestamp(L"PreparingHealthCheckStartUtcTimestamp");
    Common::WStringLiteral const Constants::PreparingHealthCheckEndUtcTimestamp(L"PreparingHealthCheckEndUtcTimestamp");
    Common::WStringLiteral const Constants::RestoringHealthCheckStartUtcTimestamp(L"RestoringHealthCheckStartUtcTimestamp");
    Common::WStringLiteral const Constants::RestoringHealthCheckEndUtcTimestamp(L"RestoringHealthCheckEndUtcTimestamp");
    Common::WStringLiteral const Constants::PreparingHealthCheckState(L"PreparingHealthCheckState");
    Common::WStringLiteral const Constants::RestoringHealthCheckState(L"RestoringHealthCheckState");
    Common::WStringLiteral const Constants::PerformPreparingHealthCheck(L"PerformPreparingHealthCheck");
    Common::WStringLiteral const Constants::PerformRestoringHealthCheck(L"PerformRestoringHealthCheck");
    Common::WStringLiteral const Constants::Impact(L"Impact");
    Common::WStringLiteral const Constants::ImpactLevel(L"ImpactLevel");
    Common::WStringLiteral const Constants::NodeImpactList(L"NodeImpactList");
    Common::WStringLiteral const Constants::NodeNames(L"NodeNames");
    Common::WStringLiteral const Constants::RequestAbort(L"RequestAbort");
    Common::WStringLiteral const Constants::MinNodeLoadValue(L"MinNodeLoadValue");
    Common::WStringLiteral const Constants::MinNodeLoadId(L"MinNodeLoadId");
    Common::WStringLiteral const Constants::MaxNodeLoadValue(L"MaxNodeLoadValue");
    Common::WStringLiteral const Constants::MaxNodeLoadId(L"MaxNodeLoadId");
    Common::WStringLiteral const Constants::CurrentClusterLoad(L"CurrentClusterLoad");
    Common::WStringLiteral const Constants::BufferedClusterCapacityRemaining(L"BufferedClusterCapacityRemaining");
    Common::WStringLiteral const Constants::ClusterCapacityRemaining(L"ClusterCapacityRemaining");
    Common::WStringLiteral const Constants::MaximumNodeLoad(L"MaximumNodeLoad");
    Common::WStringLiteral const Constants::MinimumNodeLoad(L"MinimumNodeLoad");
    Common::WStringLiteral const Constants::OnlyQueryPrimaries(L"OnlyQueryPrimaries");
    Common::WStringLiteral const Constants::UnplacedReplicaDetails(L"UnplacedReplicaDetails");
    Common::WStringLiteral const Constants::Error(L"Error");
    Common::WStringLiteral const Constants::Code(L"Code");
    Common::WStringLiteral const Constants::Message(L"Message");
    Common::WStringLiteral const Constants::ReconfigurationPhase(L"ReconfigurationPhase");
    Common::WStringLiteral const Constants::ReconfigurationType(L"ReconfigurationType");
    Common::WStringLiteral const Constants::ReconfigurationStartTimeUtc(L"ReconfigurationStartTimeUtc");
    Common::WStringLiteral const Constants::ReconfigurationInformation(L"ReconfigurationInformation");
    Common::WStringLiteral const Constants::DeployedServiceReplica(L"DeployedServiceReplica");
    Common::WStringLiteral const Constants::DeployedServiceReplicaInstance(L"DeployedServiceReplicaInstance");

    Common::WStringLiteral const Constants::VersionNumber(L"VersionNumber");
    Common::WStringLiteral const Constants::EpochDataLossNumber(L"EpochDataLossNumber");
    Common::WStringLiteral const Constants::EpochConfigurationNumber(L"EpochConfigurationNumber");
    Common::WStringLiteral const Constants::StoreFiles(L"StoreFiles");
    Common::WStringLiteral const Constants::StoreFolders(L"StoreFolders");
    Common::WStringLiteral const Constants::StoreRelativePath(L"StoreRelativePath");
    Common::WStringLiteral const Constants::FileCount(L"FileCount");
    Common::WStringLiteral const Constants::FileSize(L"FileSize");
    Common::WStringLiteral const Constants::FileVersion(L"FileVersion");
    Common::WStringLiteral const Constants::ModifiedDate(L"ModifiedDate");
    Common::WStringLiteral const Constants::RemoteLocation(L"RemoteLocation");
    Common::WStringLiteral const Constants::RemoteSource(L"RemoteSource");
    Common::WStringLiteral const Constants::RemoteDestination(L"RemoteDestination");
    Common::WStringLiteral const Constants::SkipFiles(L"SkipFiles");
    Common::WStringLiteral const Constants::CopyFlag(L"CopyFlag");
    Common::WStringLiteral const Constants::CheckMarkFile(L"CheckMarkFile");
    Common::WStringLiteral const Constants::StartPosition(L"StartPosition");
    Common::WStringLiteral const Constants::EndPosition(L"EndPosition");
    Common::WStringLiteral const Constants::SessionId(L"SessionId");
    Common::WStringLiteral const Constants::ExpectedRanges(L"ExpectedRanges");
    Common::WStringLiteral const Constants::UploadSessions(L"UploadSessions");
    Common::WStringLiteral const Constants::IsRecursive(L"IsRecursive");
    Common::WStringLiteral const Constants::ChunkContent(L"ChunkContent");

    // Health Reporting
    // HealthReport SourceId
    GlobalWString const Constants::HealthReportFMMSource = make_global<std::wstring>(L"System.FMM");
    GlobalWString const Constants::HealthReportFMSource = make_global<std::wstring>(L"System.FM");
    GlobalWString const Constants::HealthReportPLBSource = make_global<std::wstring>(L"System.PLB");
    GlobalWString const Constants::HealthReportCRMSource = make_global<std::wstring>(L"System.CRM");
    GlobalWString const Constants::HealthReportRASource = make_global<std::wstring>(L"System.RA");
    GlobalWString const Constants::HealthReportRAPSource = make_global<std::wstring>(L"System.RAP");
    GlobalWString const Constants::HealthReportFabricNodeSource = make_global<std::wstring>(L"System.FabricNode");
    GlobalWString const Constants::HealthReportCMSource = make_global<std::wstring>(L"System.CM");
    GlobalWString const Constants::HealthReportTestabilitySource = make_global<std::wstring>(L"System.Testability");
    GlobalWString const Constants::HealthReportFederationSource = make_global<std::wstring>(L"System.Federation");
    GlobalWString const Constants::HealthReportHostingSource = make_global<std::wstring>(L"System.Hosting");
    GlobalWString const Constants::HealthReportReplicatorSource = make_global<std::wstring>(L"System.Replicator");
    GlobalWString const Constants::HealthReportReplicatedStoreSource = make_global<std::wstring>(L"System.ReplicatedStore");
    GlobalWString const Constants::HealthReportNamingServiceSource = make_global<std::wstring>(L"System.NamingService");
    GlobalWString const Constants::HealthReportHMSource = make_global<std::wstring>(L"System.HM");
    GlobalWString const Constants::HealthReportTransactionalReplicatorSource = make_global<std::wstring>(L"TransactionalReplicator");

    // HealthReport Property
    GlobalWString const Constants::ReconfigurationProperty = make_global<std::wstring>(L"Reconfiguration");
    GlobalWString const Constants::RebuildProperty = make_global<std::wstring>(L"Rebuild");
    GlobalWString const Constants::HealthStateProperty = make_global<std::wstring>(L"State");
    GlobalWString const Constants::HealthActivationProperty = make_global<std::wstring>(L"Activation");
    GlobalWString const Constants::HealthCapacityProperty = make_global<std::wstring>(L"Capacity_");
    GlobalWString const Constants::ServiceReplicaUnplacedHealthProperty = make_global<std::wstring>(L"ServiceReplicaUnplacedHealth_");
    GlobalWString const Constants::ReplicaConstraintViolationProperty = make_global<std::wstring>(L"ReplicaConstraintViolation_");
    GlobalWString const Constants::FabricCertificateProperty = make_global<std::wstring>(L"Certificate_");
    GlobalWString const Constants::FabricCertificateRevocationListProperty = make_global<std::wstring>(L"CertificateRevocationList_");
    GlobalWString const Constants::SecurityApiProperty = make_global<std::wstring>(L"SecurityApi_");
    GlobalWString const Constants::NeighborhoodProperty = make_global<std::wstring>(L"Neighborhood_");
    GlobalWString const Constants::HealthReplicaOpenStatusProperty = make_global<std::wstring>(L"ReplicaOpenStatus");
    GlobalWString const Constants::HealthReplicaCloseStatusProperty = make_global<std::wstring>(L"ReplicaCloseStatus");
    GlobalWString const Constants::HealthReplicaServiceTypeRegistrationStatusProperty = make_global<std::wstring>(L"ReplicaServiceTypeRegistrationStatus");
    GlobalWString const Constants::HealthRAStoreProvider = make_global<std::wstring>(L"RAStoreProvider");
    GlobalWString const Constants::HealthReplicaChangeRoleStatusProperty = make_global<std::wstring>(L"ReplicaChangeRoleStatus");
    GlobalWString const Constants::UpgradePrimarySwapHealthProperty = make_global<std::wstring>(L"UpgradePrimarySwapFailure_");
    GlobalWString const Constants::BalancingUnsuccessfulProperty = make_global<std::wstring>(L"BalancingUnsuccessful_");
    GlobalWString const Constants::ConstraintFixUnsuccessfulProperty = make_global<std::wstring>(L"ConstraintFixUnsuccessful_");
    GlobalWString const Constants::ServiceDescriptionHealthProperty = make_global<std::wstring>(L"ServiceDescription");
    GlobalWString const Constants::CommitPerformanceHealthProperty = make_global<std::wstring>(L"CommitPerformance");
    GlobalWString const Constants::MovementEfficacyProperty = make_global<std::wstring>(L"MovementEfficacy_");
    GlobalWString const Constants::DurationHealthProperty = make_global<std::wstring>(L"Duration_");
    GlobalWString const Constants::AuthorityReportProperty = make_global<std::wstring>(L"AuthorityReport");
    GlobalWString const Constants::ResourceGovernanceReportProperty = make_global<std::wstring>(L"ResourceGovernance");
    GlobalWString const Constants::HealthReportCountProperty = make_global<std::wstring>(L"HealthReportCount");

    // KVS replica query
    Common::WStringLiteral const Constants::DatabaseRowCountEstimate(L"DatabaseRowCountEstimate");
    Common::WStringLiteral const Constants::DatabaseLogicalSizeEstimate(L"DatabaseLogicalSizeEstimate");
    Common::WStringLiteral const Constants::CopyNotificationCurrentKeyFilter(L"CopyNotificationCurrentKeyFilter");
    Common::WStringLiteral const Constants::CopyNotificationCurrentProgress(L"CopyNotificationCurrentProgress");
    Common::WStringLiteral const Constants::StatusDetails(L"StatusDetails");
    Common::WStringLiteral const Constants::MigrationStatus(L"MigrationStatus");
    Common::WStringLiteral const Constants::CurrentPhase(L"CurrentPhase");
    Common::WStringLiteral const Constants::NextPhase(L"NextPhase");
    Common::WStringLiteral const Constants::ProviderKind(L"ProviderKind");

    Common::WStringLiteral const Constants::TestCommandState(L"TestCommandState");
    Common::WStringLiteral const Constants::TestCommandType(L"TestCommandType");

    Guid const Constants::FMServiceGuid(L"00000000-0000-0000-0000-000000000001");

    GlobalWString const Constants::EmptyString = make_global<std::wstring>();

    //
    // FaultAnalysisService
    //
    Common::WStringLiteral const Constants::OperationId(L"OperationId");
    Common::WStringLiteral const Constants::PartitionSelectorType(L"PartitionSelectorType");
    Common::WStringLiteral const Constants::PartitionKey(L"PartitionKey");
    Common::WStringLiteral const Constants::RestartPartitionMode(L"RestartPartitionMode");
    Common::WStringLiteral const Constants::DataLossMode(L"DataLossMode");
    Common::WStringLiteral const Constants::QuorumLossMode(L"QuorumLossMode");
    Common::WStringLiteral const Constants::QuorumLossDurationInSeconds(L"QuorumLossDurationInSeconds");


    Common::WStringLiteral const Constants::InvokeDataLossResult(L"InvokeDataLossResult");
    Common::WStringLiteral const Constants::InvokeQuorumLossResult(L"InvokeQuorumLossResult");
    Common::WStringLiteral const Constants::RestartPartitionResult(L"RestartPartitionResult");
    Common::WStringLiteral const Constants::ErrorCode(L"ErrorCode");
    Common::WStringLiteral const Constants::SelectedPartition(L"SelectedPartition");
    Common::WStringLiteral const Constants::Force(L"Force");
    Common::WStringLiteral const Constants::NodeTransitionResult(L"NodeTransitionResult");

    Common::WStringLiteral const Constants::RestartNodeCommand(L"RestartNode");
    Common::WStringLiteral const Constants::StopNodeCommand(L"StopNode");

    //
    // UpgradeOrchestrationService
    //
    Common::WStringLiteral const Constants::ClusterConfig(L"ClusterConfig");
    Common::WStringLiteral const Constants::HealthCheckRetryTimeout(L"HealthCheckRetryTimeout");
    Common::WStringLiteral const Constants::HealthCheckWaitDurationInSeconds(L"HealthCheckWaitDurationInSeconds");
    Common::WStringLiteral const Constants::HealthCheckStableDurationInSeconds(L"HealthCheckStableDurationInSeconds");
    Common::WStringLiteral const Constants::UpgradeDomainTimeoutInSeconds(L"UpgradeDomainTimeoutInSeconds");
    Common::WStringLiteral const Constants::UpgradeTimeoutInSeconds(L"UpgradeTimeoutInSeconds");
    Common::WStringLiteral const Constants::RollbackOnFailure(L"RollbackOnFailure");
    Common::WStringLiteral const Constants::ClusterConfiguration(L"ClusterConfiguration");
    Common::WStringLiteral const Constants::ServiceState(L"ServiceState");

    Common::WStringLiteral const Constants::CurrentCodeVersion(L"CurrentCodeVersion");
    Common::WStringLiteral const Constants::CurrentManifestVersion(L"CurrentManifestVersion");
    Common::WStringLiteral const Constants::TargetCodeVersion(L"TargetCodeVersion");
    Common::WStringLiteral const Constants::TargetManifestVersion(L"TargetManifestVersion");
    Common::WStringLiteral const Constants::PendingUpgradeType(L"PendingUpgradeType");

    Common::WStringLiteral const Constants::NodeResult(L"NodeResult");

    //
    // Secret Store Service
    //
    int const Constants::SecretNameMaxLength(256);
    int const Constants::SecretVersionMaxLength(256);
    int const Constants::SecretKindMaxLength(256);
    int const Constants::SecretContentTypeMaxLength(256);
    int const Constants::SecretValueMaxSize(4 * 1024 * 1024); //4MB

    //
    // Compose Deployment
    //
    Common::WStringLiteral const Constants::RegistryUserName(L"RegistryUserName");
    Common::WStringLiteral const Constants::RegistryPassword(L"RegistryPassword");
    Common::WStringLiteral const Constants::PasswordEncrypted(L"PasswordEncrypted");
    Common::WStringLiteral const Constants::ComposeFileContent(L"ComposeFileContent");
    Common::WStringLiteral const Constants::RegistryCredential(L"RegistryCredential");
    // Deprecated
    Common::WStringLiteral const Constants::RepositoryUserName(L"RepositoryUserName");
    Common::WStringLiteral const Constants::RepositoryPassword(L"RepositoryPassword");
    Common::WStringLiteral const Constants::RepositoryCredential(L"RepositoryCredential");

    Common::WStringLiteral const Constants::ComposeDeploymentStatus(L"ComposeDeploymentStatus");

    //
    // Mesh
    //
    Common::WStringLiteral const Constants::deploymentName(L"deploymentName");
    Common::WStringLiteral const Constants::applicationName(L"applicationName");
    Common::WStringLiteral const Constants::applicationUri(L"applicationUri");
    Common::WStringLiteral const Constants::status(L"status");
    Common::WStringLiteral const Constants::statusDetails(L"statusDetails");
    Common::WStringLiteral const Constants::properties(L"properties");
    Common::WStringLiteral const Constants::instanceCount(L"instanceCount");
    Common::WStringLiteral const Constants::containerRegistryServer(L"server");
    Common::WStringLiteral const Constants::containerRegistryUserName(L"username");
    Common::WStringLiteral const Constants::containerRegistryPassword(L"password");
    Common::WStringLiteral const Constants::restartPolicy(L"restartPolicy");
    Common::WStringLiteral const Constants::volumes(L"volumes");
    Common::WStringLiteral const Constants::image(L"image");
    Common::WStringLiteral const Constants::command(L"command");
    Common::WStringLiteral const Constants::ports(L"ports");
    Common::WStringLiteral const Constants::environmentVariables(L"environmentVariables");
    Common::WStringLiteral const Constants::instanceView(L"instanceView");
    Common::WStringLiteral const Constants::restartCount(L"restartCount");
    Common::WStringLiteral const Constants::currentState(L"currentState");
    Common::WStringLiteral const Constants::previousState(L"previousState");
    Common::WStringLiteral const Constants::state(L"state");
    Common::WStringLiteral const Constants::exitCode(L"exitCode");
    Common::WStringLiteral const Constants::resources(L"resources");
    Common::WStringLiteral const Constants::resourcesRequests(L"requests");
    Common::WStringLiteral const Constants::resourcesLimits(L"limits");
    Common::WStringLiteral const Constants::volumeMounts(L"volumeMounts");
    Common::WStringLiteral const Constants::memoryInGB(L"memoryInGB");
    Common::WStringLiteral const Constants::cpu(L"cpu");
    Common::WStringLiteral const Constants::mountPath(L"mountPath");
    Common::WStringLiteral const Constants::readOnly(L"readOnly");
    Common::WStringLiteral const Constants::imageRegistryCredentials(L"imageRegistryCredentials");
    Common::WStringLiteral const Constants::port(L"port");
    Common::WStringLiteral const Constants::creationParameters(L"creationParameters");
    Common::WStringLiteral const Constants::nameCamelCase(L"name");
    Common::WStringLiteral const Constants::valueCamelCase(L"value");
    Common::WStringLiteral const Constants::azureFile(L"azureFile");
    Common::WStringLiteral const Constants::shareName(L"shareName");
    Common::WStringLiteral const Constants::storageAccountName(L"storageAccountName");
    Common::WStringLiteral const Constants::storageAccountKey(L"storageAccountKey");
    Common::WStringLiteral const Constants::accountName(L"accountName");
    Common::WStringLiteral const Constants::accountKey(L"accountKey");
    Common::WStringLiteral const Constants::sizeDisk(L"sizeDisk");
    Common::WStringLiteral const Constants::volumeName(L"volumeName");
    Common::WStringLiteral const Constants::volumeDescription(L"description");
    Common::WStringLiteral const Constants::volumeDescriptionForImageBuilder(L"volumeDescription");
    Common::WStringLiteral const Constants::volumeParameters(L"parameters");
    Common::WStringLiteral const Constants::osType(L"osType");
    Common::WStringLiteral const Constants::doNotPersistState(L"doNotPersistState");
    Common::WStringLiteral const Constants::gatewayName(L"gatewayName");

    Common::WStringLiteral const Constants::autoScalingPolicies(L"autoScalingPolicies");
    Common::WStringLiteral const Constants::autoScalingName(L"name");
    Common::WStringLiteral const Constants::autoScalingTrigger(L"trigger");
    Common::WStringLiteral const Constants::autoScalingTriggerKind(L"kind");
    Common::WStringLiteral const Constants::autoScalingMetric(L"metric");
    Common::WStringLiteral const Constants::autoScalingMetricKind(L"kind");
    Common::WStringLiteral const Constants::autoScalingMetricName(L"name");
    Common::WStringLiteral const Constants::autoScalingLowerLoadThreshold(L"lowerLoadThreshold");
    Common::WStringLiteral const Constants::autoScalingUpperLoadThreshold(L"upperLoadThreshold");
    Common::WStringLiteral const Constants::autoScaleIntervalInSeconds(L"scaleIntervalInSeconds");
    Common::WStringLiteral const Constants::autoScalingMechanism(L"mechanism");
    Common::WStringLiteral const Constants::autoScalingMechanismKind(L"kind");
    Common::WStringLiteral const Constants::autoScalingMinInstanceCount(L"minCount");
    Common::WStringLiteral const Constants::autoScalingMaxInstanceCount(L"maxCount");
    Common::WStringLiteral const Constants::autoScaleIncrement(L"scaleIncrement");

    //
    // In the mesh environment we use the given service name and application name to generate the dns name. The behavior of dns queries
    // across operating system's when 'dots' are present in the dns names is not consistent. So we prevent users from specifying 'dot' in
    // the service name and application name.
    // Eg: Linux behavior can be read here(https://linux.die.net/man/5/resolv.conf).
    //
    Common::GlobalWString const Constants::ModelV2ReservedDnsNameCharacters = make_global<std::wstring>(L".");

    Common::WStringLiteral const Constants::RemoveServiceFabricRuntimeAccess(L"RemoveServiceFabricRuntimeAccess");
    Common::WStringLiteral const Constants::AzureFilePluginName(L"AzureFilePluginName");

    Common::GlobalWString const Constants::SystemMetricNameCpuCores = make_global<std::wstring>(L"servicefabric:/_CpuCores");
    Common::GlobalWString const Constants::SystemMetricNameMemoryInMB = make_global<std::wstring>(L"servicefabric:/_MemoryInMB");
    uint const Constants::ResourceGovernanceCpuCorrectionFactor(1000000);

    Common::Global<vector<wstring>> const Constants::ValidNodeIdGeneratorVersionList = [] {
        Global<vector<wstring>> v = make_global<vector<wstring>>();
        v->push_back(L"v3");
        v->push_back(L"V3");
        v->push_back(L"v4");
        v->push_back(L"V4");
        return v;
    } ();

    //
    // Container Network
    //
    Common::WStringLiteral const Constants::NetworkType(L"NetworkType");
    Common::WStringLiteral const Constants::NetworkTypeCamelCase(L"networkType");
    Common::WStringLiteral const Constants::NetworkName(L"NetworkName");
    Common::WStringLiteral const Constants::NetworkNameCamelCase(L"networkName");
    Common::WStringLiteral const Constants::NetworkAddressPrefix(L"NetworkAddressPrefix");
    Common::WStringLiteral const Constants::NetworkAddressPrefixCamelCase(L"networkAddressPrefix");
    Common::WStringLiteral const Constants::NetworkStatus(L"NetworkStatus");
    Common::WStringLiteral const Constants::NetworkDescription(L"NetworkDescription");
    Common::WStringLiteral const Constants::CodePackageVersion(L"CodePackageVersion");
    Common::WStringLiteral const Constants::ContainerAddress(L"ContainerAddress");
    Common::WStringLiteral const Constants::ContainerId(L"ContainerId");
    Common::WStringLiteral const Constants::ContainerNetworkPolicies(L"ContainerNetworkPolicies");
    Common::WStringLiteral const Constants::NetworkRef(L"NetworkRef");
    Common::WStringLiteral const Constants::EndpointBindings(L"EndpointBindings");
    Common::WStringLiteral const Constants::EndpointRef(L"EndpointRef");
    Common::WStringLiteral const Constants::endpointRefsCamelCase(L"endpointRefs");

    //
    // ContinuationToken Constants
    //
    wstring const Constants::ContinuationTokenParserDelimiter(L"+");
    wstring const Constants::ContinuationTokenCreationFailedMessage(L"ContinuationTokenUnableToBeCreated");

    DEFINE_TEST_MUTABLE_CONSTANT(size_t, NamedPropertyMaxValueSize, 1 * 1024 * 1024)
}
