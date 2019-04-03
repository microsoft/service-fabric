// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // "Test mutable constants" are used in product code like regular static const members
    // (i.e. Constants::MemberName) and are not exposed in any way through config.
    // However, you can change their values for testing purposes using Constants::Test_SetMemberName().
    //
#define DECLARE_TEST_MUTABLE_CONSTANT(type, name) \
private: \
    static type name##_; \
public: \
    typedef struct name##_t \
    { \
    inline operator type() { return name##_; } \
    void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const; \
} name##_t; \
    static name##_t name; \
    static void Test_Set##name(type const & value) { name##_ = value; } \

#define DEFINE_TEST_MUTABLE_CONSTANT(type, name, value) \
    void Constants::name##_t::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const { w << Constants::name##_; } \
    type Constants::name##_(value); \
    Constants::name##_t Constants::name = Constants::name##_t(); \

    class Constants
    {
    public:

        //
        // Naming Service
        //
        static __int64 const UninitializedVersion;

        //
        // JSON serialization - Field Names
        //
        static Common::WStringLiteral const Name;
        static Common::WStringLiteral const DeploymentName;
        static Common::WStringLiteral const HealthStateFilter;
        static Common::WStringLiteral const TypeName;
        static Common::WStringLiteral const TypeVersion;
        static Common::WStringLiteral const ParameterList;
        static Common::WStringLiteral const DefaultParameterList;
        static Common::WStringLiteral const ApplicationHealthPolicy;
        static Common::WStringLiteral const ApplicationHealthPolicyMap;
        static Common::WStringLiteral const ApplicationHealthPolicies;
        static Common::WStringLiteral const instances;
        static Common::WStringLiteral const instanceNames;
        static Common::WStringLiteral const MaxPercentServicesUnhealthy;
        static Common::WStringLiteral const MaxPercentDeployedApplicationsUnhealthy;
        static Common::WStringLiteral const Value;
        static Common::WStringLiteral const Status;
        static Common::WStringLiteral const Parameters;
        static Common::WStringLiteral const HealthState;
        static Common::WStringLiteral const AggregatedHealthState;
        static Common::WStringLiteral const PartitionStatus;
        static Common::WStringLiteral const LastQuorumLossDurationInSeconds;
        static Common::WStringLiteral const ServiceManifestVersion;
        static Common::WStringLiteral const TargetApplicationTypeVersion;
        static Common::WStringLiteral const TargetApplicationParameterList;
        static Common::WStringLiteral const UpgradeKind;
        static Common::WStringLiteral const UpgradeDescription;
        static Common::WStringLiteral const UpdateDescription;
        static Common::WStringLiteral const RollingUpgradeMode;
        static Common::WStringLiteral const UpgradeReplicaSetCheckTimeoutInSeconds;
        static Common::WStringLiteral const ReplicaSetCheckTimeoutInMilliseconds;
        static Common::WStringLiteral const ForceRestart;
        static Common::WStringLiteral const MonitoringPolicy;
        static Common::WStringLiteral const ClusterHealthPolicy;
        static Common::WStringLiteral const EnableDeltaHealthEvaluation;
        static Common::WStringLiteral const ClusterUpgradeHealthPolicy;
        static Common::WStringLiteral const MaxPercentNodesUnhealthyPerUpgradeDomain;
        static Common::WStringLiteral const MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain;
        static Common::WStringLiteral const DefaultApplicationHealthAggregationPolicy;
        static Common::WStringLiteral const ApplicationHealthAggregationPolicyOverrides;
        static Common::WStringLiteral const MaxPercentNodesUnhealthy;
        static Common::WStringLiteral const MaxPercentDeltaUnhealthyNodes;
        static Common::WStringLiteral const MaxPercentUpgradeDomainDeltaUnhealthyNodes;
        static Common::WStringLiteral const EntryPointLocation;
        static Common::WStringLiteral const ProcessId;
        static Common::WStringLiteral const HostProcessId;
        static Common::WStringLiteral const LastExitCode;
        static Common::WStringLiteral const LastActivationTime;
        static Common::WStringLiteral const LastExitTime;
        static Common::WStringLiteral const LastSuccessfulActivationTime;
        static Common::WStringLiteral const LastSuccessfulExitTime;
        static Common::WStringLiteral const ActivationFailureCount;
        static Common::WStringLiteral const ContinuousActivationFailureCount;
        static Common::WStringLiteral const ExitFailureCount;
        static Common::WStringLiteral const ContinuousExitFailureCount;
        static Common::WStringLiteral const ActivationCount;
        static Common::WStringLiteral const ExitCount;
        static Common::WStringLiteral const SetupEntryPoint;
        static Common::WStringLiteral const MainEntryPoint;
        static Common::WStringLiteral const HasSetupEntryPoint;
        static Common::WStringLiteral const ServiceKind;
        static Common::WStringLiteral const ReplicaId;
        static Common::WStringLiteral const ReplicaOrInstanceId;
        static Common::WStringLiteral const ReplicaRole;
        static Common::WStringLiteral const PreviousConfigurationRole;
        static Common::WStringLiteral const InstanceId;
        static Common::WStringLiteral const ReplicaStatus;
        static Common::WStringLiteral const ReplicaState;
        static Common::WStringLiteral const LastInBuildDurationInSeconds;
        static Common::WStringLiteral const PartitionId;
        static Common::WStringLiteral const ActivationId;
        static Common::WStringLiteral const ServiceName;
        static Common::WStringLiteral const Address;
        static Common::WStringLiteral const CodePackageName;
        static Common::WStringLiteral const ServiceTypeName;
        static Common::WStringLiteral const ServiceGroupMemberDescription;
        static Common::WStringLiteral const IsServiceGroup;
        static Common::WStringLiteral const CodeVersion;
        static Common::WStringLiteral const ConfigVersion;
        static Common::WStringLiteral const Details;
        static Common::WStringLiteral const RunFrequencyInterval;
        static Common::WStringLiteral const ConsiderWarningAsError;
        static Common::WStringLiteral const IgnoreExpiredEvents;
        static Common::WStringLiteral const ApplicationName;
        static Common::WStringLiteral const InitializationData;
        static Common::WStringLiteral const PartitionDescription;
        static Common::WStringLiteral const TargetReplicaSetSize;
        static Common::WStringLiteral const MinReplicaSetSize;
        static Common::WStringLiteral const HasPersistedState;
        static Common::WStringLiteral const InstanceCount;
        static Common::WStringLiteral const PlacementConstraints;
        static Common::WStringLiteral const CorrelationScheme;
        static Common::WStringLiteral const ServiceLoadMetrics;
        static Common::WStringLiteral const DefaultMoveCost;
        static Common::WStringLiteral const IsDefaultMoveCostSpecified;
        static Common::WStringLiteral const ServicePackageActivationMode;
        static Common::WStringLiteral const ServicePlacementPolicies;
        static Common::WStringLiteral const Flags;
        static Common::WStringLiteral const ReplicaRestartWaitDurationSeconds;
        static Common::WStringLiteral const QuorumLossWaitDurationSeconds;
        static Common::WStringLiteral const StandByReplicaKeepDurationSeconds;
        static Common::WStringLiteral const ReplicaRestartWaitDurationInMilliseconds;
        static Common::WStringLiteral const QuorumLossWaitDurationInMilliseconds;
        static Common::WStringLiteral const StandByReplicaKeepDurationInMilliseconds;
        static Common::WStringLiteral const Id;
        static Common::WStringLiteral const IpAddressOrFQDN;
        static Common::WStringLiteral const Type;
        static Common::WStringLiteral const Version;
        static Common::WStringLiteral const NodeStatus;
        static Common::WStringLiteral const NodeDeactivationIntent;
        static Common::WStringLiteral const NodeDeactivationStatus;
        static Common::WStringLiteral const NodeDeactivationTask;
        static Common::WStringLiteral const NodeDeactivationTaskId;
        static Common::WStringLiteral const NodeDeactivationTaskType;
        static Common::WStringLiteral const NodeDeactivationTaskIdPrefixRM;
        static Common::WStringLiteral const NodeDeactivationTaskIdPrefixWindowsAzure;
        static Common::WStringLiteral const NodeDeactivationInfo;
        static Common::WStringLiteral const NodeUpTimeInSeconds;
        static Common::WStringLiteral const NodeDownTimeInSeconds;
        static Common::WStringLiteral const NodeUpAt;
        static Common::WStringLiteral const NodeDownAt;
        static Common::WStringLiteral const UpgradeDomain;
        static Common::WStringLiteral const FaultDomain;
        static Common::WStringLiteral const PartitionScheme;
        static Common::WStringLiteral const Count;
        static Common::WStringLiteral const Names;
        static Common::WStringLiteral const LowKey;
        static Common::WStringLiteral const HighKey;
        static Common::WStringLiteral const MaxPercentReplicasUnhealthy;
        static Common::WStringLiteral const Scheme;
        static Common::WStringLiteral const Weight;
        static Common::WStringLiteral const PrimaryDefaultLoad;
        static Common::WStringLiteral const SecondaryDefaultLoad;
        static Common::WStringLiteral const DefaultLoad;
        static Common::WStringLiteral const FailureAction;
        static Common::WStringLiteral const HealthCheckWaitDurationInMilliseconds;
        static Common::WStringLiteral const HealthCheckStableDurationInMilliseconds;
        static Common::WStringLiteral const HealthCheckRetryTimeoutInMilliseconds;
        static Common::WStringLiteral const UpgradeTimeoutInMilliseconds;
        static Common::WStringLiteral const UpgradeDomainTimeoutInMilliseconds;
        static Common::WStringLiteral const UpgradeDuration;
        static Common::WStringLiteral const UpgradeDurationInMilliseconds;
        static Common::WStringLiteral const CurrentUpgradeDomainDuration;
        static Common::WStringLiteral const UpgradeDomainDurationInMilliseconds;
        static Common::WStringLiteral const MaxPercentPartitionsUnhealthy;
        static Common::WStringLiteral const RepartitionDescription;
        static Common::WStringLiteral const PartitionKind;
        static Common::WStringLiteral const NamesToAdd;
        static Common::WStringLiteral const NamesToRemove;
        static Common::WStringLiteral const ServicePartitionKind;
        static Common::WStringLiteral const PartitionInformation;
        static Common::WStringLiteral const InQuorumLoss;
        static Common::WStringLiteral const DomainName;
        static Common::WStringLiteral const ManifestVersion;
        static Common::WStringLiteral const NodeName;
        static Common::WStringLiteral const NodeNameCamelCase;
        static Common::WStringLiteral const NodeId;
        static Common::WStringLiteral const NodeInstanceId;
        static Common::WStringLiteral const Content;
        static Common::WStringLiteral const UriPath;
        static Common::WStringLiteral const HttpVerb;
        static Common::WStringLiteral const HttpStatus;
        static Common::WStringLiteral const HttpRequestBody;
        static Common::WStringLiteral const HttpResponseBody;
        static Common::WStringLiteral const HttpContentType;
        static Common::WStringLiteral const HttpContentEncoding;
        static Common::WStringLiteral const CreateFabricDump;
        static Common::WStringLiteral const IsStateful;
        static Common::WStringLiteral const UseImplicitHost;
        static Common::WStringLiteral const UseImplicitFactory;
        static Common::WStringLiteral const UseServiceFabricReplicatedStore;
        static Common::WStringLiteral const Extensions;
        static Common::WStringLiteral const LoadMetrics;
        static Common::WStringLiteral const ServiceTypeDescription;
        static Common::WStringLiteral const ServiceGroupTypeDescription;
        static Common::WStringLiteral const ServiceGroupTypeMemberDescription;
        static Common::WStringLiteral const IsSeedNode;
        static Common::WStringLiteral const ReplicaRestartWaitDurationInMilliSeconds;
        static Common::WStringLiteral const RunAsUserName;
        static Common::WStringLiteral const CodePackageEntryPointStatistics;
        static Common::WStringLiteral const Manifest;
        static Common::WStringLiteral const ServiceManifestName;
        static Common::WStringLiteral const ServicePackageActivationId;
        static Common::WStringLiteral const HostType;
        static Common::WStringLiteral const HostIsolationMode;
        static Common::WStringLiteral const ApplicationTypeBuildPath;
        static Common::WStringLiteral const ApplicationPackageDownloadUri;
        static Common::WStringLiteral const ApplicationPackageCleanupPolicy;
        static Common::WStringLiteral const Async;
        static Common::WStringLiteral const NextUpgradeDomain;
        static Common::WStringLiteral const State;
        static Common::WStringLiteral const CodeFilePath;
        static Common::WStringLiteral const ClusterManifestFilePath;
        static Common::WStringLiteral const DeactivationIntent;
        static Common::WStringLiteral const ApplicationTypeVersion;
        static Common::WStringLiteral const UpgradeDomains;
        static Common::WStringLiteral const UpgradeState;
        static Common::WStringLiteral const ProgressStatus;
        static Common::WStringLiteral const CodePackageInstanceId;
        static Common::WStringLiteral const UpgradeDomainName;
        static Common::WStringLiteral const MaxPercentUnhealthyServices;
        static Common::WStringLiteral const MaxPercentUnhealthyPartitionsPerService;
        static Common::WStringLiteral const MaxPercentUnhealthyReplicasPerPartition;
        static Common::WStringLiteral const MaxPercentUnhealthyDeployedApplications;
        static Common::WStringLiteral const DefaultServiceTypeHealthPolicy;
        static Common::WStringLiteral const ServiceTypeHealthPolicyMap;
        static Common::WStringLiteral const MaxPercentUnhealthyNodes;
        static Common::WStringLiteral const MaxPercentUnhealthyApplications;
        static Common::WStringLiteral const TotalCount;
        static Common::WStringLiteral const BaselineTotalCount;
        static Common::WStringLiteral const BaselineErrorCount;
        static Common::WStringLiteral const SourceId;
        static Common::WStringLiteral const Property;
        static Common::WStringLiteral const Description;
        static Common::WStringLiteral const SequenceNumber;
        static Common::WStringLiteral const RemoveWhenExpired;
        static Common::WStringLiteral const TimeToLiveInMs;
        static Common::WStringLiteral const SourceUtcTimestamp;
        static Common::WStringLiteral const LastModifiedUtcTimestamp;
        static Common::WStringLiteral const LastOkTransitionAt;
        static Common::WStringLiteral const LastWarningTransitionAt;
        static Common::WStringLiteral const LastErrorTransitionAt;
        static Common::WStringLiteral const IsExpired;
        static Common::WStringLiteral const HealthEvents;
        static Common::WStringLiteral const HealthEvent;
        static Common::WStringLiteral const OkCount;
        static Common::WStringLiteral const WarningCount;
        static Common::WStringLiteral const ErrorCount;
        static Common::WStringLiteral const EntityKind;
        static Common::WStringLiteral const HealthStateCount;
        static Common::WStringLiteral const HealthStatistics;
        static Common::WStringLiteral const HealthStateCountList;
        static Common::WStringLiteral const ExcludeHealthStatistics;
        static Common::WStringLiteral const IncludeSystemApplicationHealthStatistics;
        static Common::WStringLiteral const ServiceAggregatedHealthStates;
        static Common::WStringLiteral const DeployedApplicationsAggregatedHealthStates;
        static Common::WStringLiteral const PartitionAggregatedHealthState;
        static Common::WStringLiteral const ReplicaAggregatedHealthState;
        static Common::WStringLiteral const DeployedServicePackageAggregatedHealthState;
        static Common::WStringLiteral const NextActivationTime;
        static Common::WStringLiteral const NodeAggregatedHealthStates;
        static Common::WStringLiteral const ApplicationAggregatedHealthStates;
        static Common::WStringLiteral const SystemApplicationAggregatedHealthState;
        static Common::WStringLiteral const ServiceManifestNameFilter;
        static Common::WStringLiteral const ServicePackageActivationIdFilter;
        static Common::WStringLiteral const ApplicationNameFilter;
        static Common::WStringLiteral const ApplicationTypeNameFilter;
        static Common::WStringLiteral const ApplicationDefinitionKind;
        static Common::WStringLiteral const ApplicationTypeDefinitionKind;
        static Common::WStringLiteral const DeployedServicePackagesFilter;
        static Common::WStringLiteral const NodeNameFilter;
        static Common::WStringLiteral const ReplicaOrInstanceIdFilter;
        static Common::WStringLiteral const PartitionIdFilter;
        static Common::WStringLiteral const ReplicasFilter;
        static Common::WStringLiteral const PartitionsFilter;
        static Common::WStringLiteral const ServiceNameFilter;
        static Common::WStringLiteral const DeployedApplicationsFilter;
        static Common::WStringLiteral const ServicesFilter;
        static Common::WStringLiteral const ApplicationsFilter;
        static Common::WStringLiteral const HealthPolicy;
        static Common::WStringLiteral const NodesFilter;
        static Common::WStringLiteral const DeployedServicePackageFilters;
        static Common::WStringLiteral const ReplicaFilters;
        static Common::WStringLiteral const PartitionFilters;
        static Common::WStringLiteral const DeployedApplicationFilters;
        static Common::WStringLiteral const ServiceFilters;
        static Common::WStringLiteral const ApplicationFilters;
        static Common::WStringLiteral const NodeFilters;
        static Common::WStringLiteral const ReplicaHealthStateChunks;
        static Common::WStringLiteral const PartitionHealthStateChunks;
        static Common::WStringLiteral const DeployedServicePackageHealthStateChunks;
        static Common::WStringLiteral const DeployedApplicationHealthStateChunks;
        static Common::WStringLiteral const ServiceHealthStateChunks;
        static Common::WStringLiteral const ApplicationHealthStateChunks;
        static Common::WStringLiteral const NodeHealthStateChunks;
        static Common::WStringLiteral const Metadata;
        static Common::WStringLiteral const ServiceDnsName;
        static Common::WStringLiteral const ScalingPolicies;
        static Common::WStringLiteral const Key;
        static Common::WStringLiteral const LastBalancingStartTimeUtc;
        static Common::WStringLiteral const LastBalancingEndTimeUtc;
        static Common::WStringLiteral const LoadMetricInformation;
        static Common::WStringLiteral const IsBalancedBefore;
        static Common::WStringLiteral const IsBalancedAfter;
        static Common::WStringLiteral const DeviationBefore;
        static Common::WStringLiteral const DeviationAfter;
        static Common::WStringLiteral const ActivityThreshold;
        static Common::WStringLiteral const BalancingThreshold;
        static Common::WStringLiteral const ClusterCapacity;
        static Common::WStringLiteral const ClusterLoad;
        static Common::WStringLiteral const RemainingUnbufferedCapacity;
        static Common::WStringLiteral const NodeBufferPercentage;
        static Common::WStringLiteral const BufferedCapacity;
        static Common::WStringLiteral const RemainingBufferedCapacity;
        static Common::WStringLiteral const IsClusterCapacityViolation;
        static Common::WStringLiteral const Action;
        static Common::WStringLiteral const NodeCapacity;
        static Common::WStringLiteral const NodeLoad;
        static Common::WStringLiteral const NodeRemainingCapacity;
        static Common::WStringLiteral const IsCapacityViolation;
        static Common::WStringLiteral const NodeBufferedCapacity;
        static Common::WStringLiteral const NodeRemainingBufferedCapacity;
        static Common::WStringLiteral const CurrentNodeLoad;
        static Common::WStringLiteral const NodeCapacityRemaining;
        static Common::WStringLiteral const BufferedNodeCapacityRemaining;
        static Common::WStringLiteral const NodeLoadMetricInformation;
        static Common::WStringLiteral const LoadMetricReports;
        static Common::WStringLiteral const PrimaryLoadMetricReports;
        static Common::WStringLiteral const SecondaryLoadMetricReports;
        static Common::WStringLiteral const NodeCapacities;
        static Common::WStringLiteral const ServiceStatus;
        static Common::WStringLiteral const Kind;
        static Common::WStringLiteral const KindLowerCase;
        static Common::WStringLiteral const UnhealthyEvent;
        static Common::WStringLiteral const unhealthyEvaluation;
        static Common::WStringLiteral const UnhealthyEvaluations;
        static Common::WStringLiteral const ApplicationUnhealthyEvaluations;
        static Common::WStringLiteral const HealthEvaluation;
        static Common::WStringLiteral const UpgradePhase;
        static Common::WStringLiteral const SafetyCheckKind;
        static Common::WStringLiteral const SafetyCheck;
        static Common::WStringLiteral const PendingSafetyChecks;
        static Common::WStringLiteral const NodeUpgradeProgressList;
        static Common::WStringLiteral const CurrentUpgradeDomainProgress;
        static Common::WStringLiteral const StartTimestampUtc;
        static Common::WStringLiteral const FailureTimestampUtc;
        static Common::WStringLiteral const FailureReason;
        static Common::WStringLiteral const UpgradeDomainProgressAtFailure;
        static Common::WStringLiteral const UpgradeStatusDetails;
        static Common::WStringLiteral const ApplicationUpgradeStatusDetails;
        static Common::WStringLiteral const WorkDirectory;
        static Common::WStringLiteral const TempDirectory;
        static Common::WStringLiteral const LogDirectory;
        static Common::WStringLiteral const CurrentServiceOperation;
        static Common::WStringLiteral const CurrentServiceOperationStartTimeUtc;
        static Common::WStringLiteral const ReportedLoad;
        static Common::WStringLiteral const CurrentValue;
        static Common::WStringLiteral const LastReportedUtc;
        static Common::WStringLiteral const CurrentReplicatorOperation;
        static Common::WStringLiteral const CurrentReplicatorOperationStartTimeUtc;
        static Common::WStringLiteral const ReadStatus;
        static Common::WStringLiteral const WriteStatus;
        static Common::WStringLiteral const ApplicationTypeName;
        static Common::WStringLiteral const CodePackageIds;
        static Common::WStringLiteral const ConfigPackageIds;
        static Common::WStringLiteral const DataPackageIds;
        static Common::WStringLiteral const ServicePackageInstanceId;
        static Common::WStringLiteral const ReplicaInstanceId;
        static Common::WStringLiteral const PackageSharingScope;
        static Common::WStringLiteral const SharedPackageName;
        static Common::WStringLiteral const PackageSharingPolicy;
        static Common::WStringLiteral const FMVersion;
        static Common::WStringLiteral const StoreVersion;
        static Common::WStringLiteral const GenerationNumber;
        static Common::WStringLiteral const Generation;
        static Common::WStringLiteral const Endpoints;
        static Common::WStringLiteral const PreviousRspVersion;
        static Common::WStringLiteral const CurrentConfigurationEpoch;
        static Common::WStringLiteral const PrimaryEpoch;
        static Common::WStringLiteral const ConfigurationVersion;
        static Common::WStringLiteral const DataLossVersion;
        static Common::WStringLiteral const ContinuationToken;
        static Common::WStringLiteral const MaxResults;
        static Common::WStringLiteral const Items;
        static Common::WStringLiteral const ApplicationTypeHealthPolicyMap;
        static Common::WStringLiteral const ApplicationCapacity;
        static Common::WStringLiteral const MinimumNodes;
        static Common::WStringLiteral const MaximumNodes;
        static Common::WStringLiteral const ApplicationMetrics;
        static Common::WStringLiteral const ReservationCapacity;
        static Common::WStringLiteral const MaximumCapacity;
        static Common::WStringLiteral const TotalApplicationCapacity;
        static Common::WStringLiteral const RemoveApplicationCapacity;
        static Common::WStringLiteral const ApplicationLoad;
        static Common::WStringLiteral const ApplicationLoadMetricInformation;
        static Common::WStringLiteral const NodeCount;
        static Common::WStringLiteral const IsStopped;
        static Common::WStringLiteral const IsConsistent;
        static Common::WStringLiteral const SubNames;
        static Common::WStringLiteral const PropertyName;
        static Common::WStringLiteral const TypeId;
        static Common::WStringLiteral const CustomTypeId;
        static Common::WStringLiteral const Parent;
        static Common::WStringLiteral const SizeInBytes;
        static Common::WStringLiteral const Properties;
        static Common::WStringLiteral const Data;
        static Common::WStringLiteral const IncludeValue;
        static Common::WStringLiteral const Exists;
        static Common::WStringLiteral const Operations;
        static Common::WStringLiteral const ErrorMessage;
        static Common::WStringLiteral const OperationIndex;

        static Common::WStringLiteral const QueryPagingDescription;

        static Common::WStringLiteral const ChaosEvent;
        static Common::WStringLiteral const ChaosEventsFilter;
        static Common::WStringLiteral const ChaosParameters;
        static Common::WStringLiteral const ClientType;
        static Common::WStringLiteral const Reason;
        static Common::WStringLiteral const Faults;
        static Common::WStringLiteral const MaxClusterStabilizationTimeoutInSeconds;
        static Common::WStringLiteral const MaxConcurrentFaults;
        static Common::WStringLiteral const WaitTimeBetweenIterationsInSeconds;
        static Common::WStringLiteral const WaitTimeBetweenFaultsInSeconds;
        static Common::WStringLiteral const TimeToRunInSeconds;
        static Common::WStringLiteral const EnableMoveReplicaFaults;
        static Common::WStringLiteral const Context;
        static Common::WStringLiteral const Map;
        static Common::WStringLiteral const TimeStampUtc;
        static Common::WStringLiteral const NodeTypeInclusionList;
        static Common::WStringLiteral const ApplicationInclusionList;
        static Common::WStringLiteral const ChaosTargetFilter;
        static Common::WStringLiteral const Schedule;
        static Common::WStringLiteral const ChaosStatus;
        static Common::WStringLiteral const ChaosScheduleStatus;
        static Common::WStringLiteral const StartDate;
        static Common::WStringLiteral const ExpiryDate;
        static Common::WStringLiteral const StartTime;
        static Common::WStringLiteral const EndTime;
        static Common::WStringLiteral const StartTimeUtc;
        static Common::WStringLiteral const EndTimeUtc;
        static Common::WStringLiteral const ChaosParametersMap;
        static Common::WStringLiteral const Jobs;
        static Common::WStringLiteral const Days;
        static Common::WStringLiteral const Times;
        static Common::WStringLiteral const Hour;
        static Common::WStringLiteral const Minute;
        static Common::WStringLiteral const Sunday;
        static Common::WStringLiteral const Monday;
        static Common::WStringLiteral const Tuesday;
        static Common::WStringLiteral const Wednesday;
        static Common::WStringLiteral const Thursday;
        static Common::WStringLiteral const Friday;
        static Common::WStringLiteral const Saturday;

        static Common::WStringLiteral const Scope;
        static Common::WStringLiteral const TaskId;
        static Common::WStringLiteral const Target;
        static Common::WStringLiteral const Executor;
        static Common::WStringLiteral const ExecutorData;
        static Common::WStringLiteral const ResultStatus;
        static Common::WStringLiteral const ResultCode;
        static Common::WStringLiteral const ResultDetails;
        static Common::WStringLiteral const History;
        static Common::WStringLiteral const CreatedUtcTimestamp;
        static Common::WStringLiteral const ClaimedUtcTimestamp;
        static Common::WStringLiteral const PreparingUtcTimestamp;
        static Common::WStringLiteral const ApprovedUtcTimestamp;
        static Common::WStringLiteral const ExecutingUtcTimestamp;
        static Common::WStringLiteral const RestoringUtcTimestamp;
        static Common::WStringLiteral const CompletedUtcTimestamp;
        static Common::WStringLiteral const PreparingHealthCheckStartUtcTimestamp;
        static Common::WStringLiteral const PreparingHealthCheckEndUtcTimestamp;
        static Common::WStringLiteral const RestoringHealthCheckStartUtcTimestamp;
        static Common::WStringLiteral const RestoringHealthCheckEndUtcTimestamp;
        static Common::WStringLiteral const PreparingHealthCheckState;
        static Common::WStringLiteral const RestoringHealthCheckState;
        static Common::WStringLiteral const PerformPreparingHealthCheck;
        static Common::WStringLiteral const PerformRestoringHealthCheck;
        static Common::WStringLiteral const Impact;
        static Common::WStringLiteral const ImpactLevel;
        static Common::WStringLiteral const NodeImpactList;
        static Common::WStringLiteral const NodeNames;
        static Common::WStringLiteral const RequestAbort;
        static Common::WStringLiteral const MinNodeLoadValue;
        static Common::WStringLiteral const MinNodeLoadId;
        static Common::WStringLiteral const MaxNodeLoadValue;
        static Common::WStringLiteral const MaxNodeLoadId;
        static Common::WStringLiteral const CurrentClusterLoad;
        static Common::WStringLiteral const BufferedClusterCapacityRemaining;
        static Common::WStringLiteral const ClusterCapacityRemaining;
        static Common::WStringLiteral const MaximumNodeLoad;
        static Common::WStringLiteral const MinimumNodeLoad;
        static Common::WStringLiteral const OnlyQueryPrimaries;
        static Common::WStringLiteral const UnplacedReplicaDetails;
        static Common::WStringLiteral const Error;
        static Common::WStringLiteral const Code;
        static Common::WStringLiteral const Message;
        static Common::WStringLiteral const ReconfigurationPhase;
        static Common::WStringLiteral const ReconfigurationType;
        static Common::WStringLiteral const ReconfigurationStartTimeUtc;
        static Common::WStringLiteral const ReconfigurationInformation;
        static Common::WStringLiteral const DeployedServiceReplica;
        static Common::WStringLiteral const DeployedServiceReplicaInstance;

        //Auto scaling
        static Common::WStringLiteral const MetricName;
        static Common::WStringLiteral const MinInstanceCount;
        static Common::WStringLiteral const MaxInstanceCount;
        static Common::WStringLiteral const MaxPartitionCount;
        static Common::WStringLiteral const MinPartitionCount;
        static Common::WStringLiteral const LowerLoadThreshold;
        static Common::WStringLiteral const UpperLoadThreshold;
        static Common::WStringLiteral const UseOnlyPrimaryLoad;
        static Common::WStringLiteral const ScaleIntervalInSeconds;
        static Common::WStringLiteral const ScaleIncrement;
        static Common::WStringLiteral const ScalingTrigger;
        static Common::WStringLiteral const ScalingMechanism;

        // Replicator Query Specific
        static Common::WStringLiteral const ReplicatorStatus;

        static Common::WStringLiteral const PrimaryReplicatorStatus;

        static Common::WStringLiteral const SecondaryReplicatorStatus;
        static Common::WStringLiteral const CopyQueueStatus;
        static Common::WStringLiteral const LastReplicationOperationReceivedTimeUtc;
        static Common::WStringLiteral const LastCopyOperationReceivedTimeUtc;
        static Common::WStringLiteral const LastAcknowledgementSentTimeUtc;

        static Common::WStringLiteral const RemoteReplicators;
        static Common::WStringLiteral const LastReceivedReplicationSequenceNumber;
        static Common::WStringLiteral const LastAppliedReplicationSequenceNumber;
        static Common::WStringLiteral const IsInBuild;
        static Common::WStringLiteral const LastReceivedCopySequenceNumber;
        static Common::WStringLiteral const LastAppliedCopySequenceNumber;
        static Common::WStringLiteral const LastAcknowledgementProcessedTimeUtc;

        static Common::WStringLiteral const RemoteReplicatorAckStatus;
        static Common::WStringLiteral const AverageReceiveDuration;
        static Common::WStringLiteral const AverageApplyDuration;
        static Common::WStringLiteral const NotReceivedCount;
        static Common::WStringLiteral const ReceivedAndNotAppliedCount;
        static Common::WStringLiteral const CopyStreamAcknowledgementDetail;
        static Common::WStringLiteral const ReplicationStreamAcknowledgementDetail;

        static Common::WStringLiteral const ReplicationQueueStatus;
        static Common::WStringLiteral const QueueUtilizationPercentage;
        static Common::WStringLiteral const FirstSequenceNumber;
        static Common::WStringLiteral const CompletedSequenceNumber;
        static Common::WStringLiteral const CommittedSequenceNumber;
        static Common::WStringLiteral const LastSequenceNumber;
        static Common::WStringLiteral const QueueMemorySize;

        static Common::WStringLiteral const VersionNumber;
        static Common::WStringLiteral const EpochDataLossNumber;
        static Common::WStringLiteral const EpochConfigurationNumber;
        static Common::WStringLiteral const StoreFiles;
        static Common::WStringLiteral const StoreFolders;
        static Common::WStringLiteral const StoreRelativePath;
        static Common::WStringLiteral const FileCount;
        static Common::WStringLiteral const FileSize;
        static Common::WStringLiteral const FileVersion;
        static Common::WStringLiteral const ModifiedDate;
        static Common::WStringLiteral const RemoteLocation;
        static Common::WStringLiteral const RemoteSource;
        static Common::WStringLiteral const RemoteDestination;
        static Common::WStringLiteral const SkipFiles;
        static Common::WStringLiteral const CopyFlag;
        static Common::WStringLiteral const CheckMarkFile;
        static Common::WStringLiteral const StartPosition;
        static Common::WStringLiteral const EndPosition;
        static Common::WStringLiteral const SessionId;
        static Common::WStringLiteral const ExpectedRanges;
        static Common::WStringLiteral const UploadSessions;
        static Common::WStringLiteral const IsRecursive;
        static Common::WStringLiteral const ChunkContent;

        static Common::GlobalWString EventSystemSourcePrefix;

        static Common::WStringLiteral const Port;
        static Common::WStringLiteral const useDynamicHostPort;
        static Common::WStringLiteral const ClusterConnectionPort;

        static Common::WStringLiteral const ApplicationIds;

        static Common::WStringLiteral const TestCommandState;
        static Common::WStringLiteral const TestCommandType;

        //
        // ServiceTypes
        //
        static Common::GlobalWString InfrastructureServicePrimaryCountName;
        static Common::GlobalWString InfrastructureServiceReplicaCountName;

        //
        // Token validation service action.
        //
        static Common::GlobalWString const ValidateTokenAction;

        //
        // Health reporting.
        //

        // Sources
        static Common::GlobalWString const HealthReportFMMSource;
        static Common::GlobalWString const HealthReportFMSource;
        static Common::GlobalWString const HealthReportPLBSource;
        static Common::GlobalWString const HealthReportCRMSource;
        static Common::GlobalWString const HealthReportReplicatorSource;
        static Common::GlobalWString const HealthReportReplicatedStoreSource;
        static Common::GlobalWString const HealthReportRASource;
        static Common::GlobalWString const HealthReportRAPSource;
        static Common::GlobalWString const HealthReportFabricNodeSource;
        static Common::GlobalWString const HealthReportFederationSource;
        static Common::GlobalWString const HealthReportHostingSource;
        static Common::GlobalWString const HealthReportCMSource;
        static Common::GlobalWString const HealthReportTestabilitySource;
        static Common::GlobalWString const HealthReportNamingServiceSource;
        static Common::GlobalWString const HealthReportHMSource;
        static Common::GlobalWString const HealthReportTransactionalReplicatorSource;

        // Properties
        static Common::GlobalWString const HealthStateProperty;
        static Common::GlobalWString const HealthActivationProperty;
        static Common::GlobalWString const HealthCapacityProperty;
        static Common::GlobalWString const ServiceReplicaUnplacedHealthProperty;
        static Common::GlobalWString const ReplicaConstraintViolationProperty;
        static Common::GlobalWString const FabricCertificateProperty;
        static Common::GlobalWString const FabricCertificateRevocationListProperty;
        static Common::GlobalWString const SecurityApiProperty;
        static Common::GlobalWString const NeighborhoodProperty;
        static Common::GlobalWString const HealthReplicaOpenStatusProperty;
        static Common::GlobalWString const HealthReplicaCloseStatusProperty;
        static Common::GlobalWString const HealthReplicaServiceTypeRegistrationStatusProperty;
        static Common::GlobalWString const HealthRAStoreProvider;
        static Common::GlobalWString const HealthReplicaChangeRoleStatusProperty;
        static Common::GlobalWString const UpgradePrimarySwapHealthProperty;
        static Common::GlobalWString const BalancingUnsuccessfulProperty;
        static Common::GlobalWString const ConstraintFixUnsuccessfulProperty;
        static Common::GlobalWString const ServiceDescriptionHealthProperty;
        static Common::GlobalWString const CommitPerformanceHealthProperty;
        static Common::GlobalWString const MovementEfficacyProperty;
        static Common::GlobalWString const DurationHealthProperty;
        static Common::GlobalWString const AuthorityReportProperty;
        static Common::GlobalWString const ResourceGovernanceReportProperty;
        static Common::GlobalWString const ReconfigurationProperty;
        static Common::GlobalWString const RebuildProperty;
        static Common::GlobalWString const HealthReportCountProperty;

        // KVS replica query
        static Common::WStringLiteral const DatabaseRowCountEstimate;
        static Common::WStringLiteral const DatabaseLogicalSizeEstimate;
        static Common::WStringLiteral const CopyNotificationCurrentKeyFilter;
        static Common::WStringLiteral const CopyNotificationCurrentProgress;
        static Common::WStringLiteral const StatusDetails;
        static Common::WStringLiteral const MigrationStatus;
        static Common::WStringLiteral const CurrentPhase;
        static Common::WStringLiteral const NextPhase;
        static Common::WStringLiteral const ProviderKind;

        //
        // Reliability
        //
        static Common::Guid const FMServiceGuid;

        static Common::GlobalWString const EmptyString;

        //
        // FaultAnalysisService
        //
        static Common::WStringLiteral const OperationId;
        static Common::WStringLiteral const PartitionSelectorType;
        static Common::WStringLiteral const PartitionKey;
        static Common::WStringLiteral const RestartPartitionMode;
        static Common::WStringLiteral const DataLossMode;
        static Common::WStringLiteral const QuorumLossMode;
        static Common::WStringLiteral const QuorumLossDurationInSeconds;

        static Common::WStringLiteral const ErrorCode;

        static Common::WStringLiteral const InvokeDataLossResult;
        static Common::WStringLiteral const InvokeQuorumLossResult;
        static Common::WStringLiteral const RestartPartitionResult;
        static Common::WStringLiteral const NodeTransitionResult;

        static Common::WStringLiteral const SelectedPartition;
        static Common::WStringLiteral const Force;

        static Common::WStringLiteral const RestartNodeCommand;
        static Common::WStringLiteral const StopNodeCommand;

        //
        // UpgradeOrchestrationService
        //
        static Common::WStringLiteral const ClusterConfig;
        static Common::WStringLiteral const RollbackOnFailure;
        static Common::WStringLiteral const HealthCheckRetryTimeout;
        static Common::WStringLiteral const HealthCheckWaitDurationInSeconds;
        static Common::WStringLiteral const HealthCheckStableDurationInSeconds;
        static Common::WStringLiteral const UpgradeDomainTimeoutInSeconds;
        static Common::WStringLiteral const UpgradeTimeoutInSeconds;
        static Common::WStringLiteral const ClusterConfiguration;
        static Common::WStringLiteral const ServiceState;

        static Common::WStringLiteral const CurrentCodeVersion;
        static Common::WStringLiteral const CurrentManifestVersion;
        static Common::WStringLiteral const TargetCodeVersion;
        static Common::WStringLiteral const TargetManifestVersion;
        static Common::WStringLiteral const PendingUpgradeType;

        static Common::WStringLiteral const NodeResult;

        //
        // Secret Store Service
        //

        // The max length of Secret Name in characters.
        static int const SecretNameMaxLength;

        // The max length of Secret Version in characters.
        static int const SecretVersionMaxLength;

        // The max length of Secret Kind in characters.
        static int const SecretKindMaxLength;

        // The max length of Secret Content Type in characters.
        static int const SecretContentTypeMaxLength;

        // The max size of Secret Value in bytes.
        static int const SecretValueMaxSize;

        //
        // Compose deployment
        //
        static Common::WStringLiteral const RegistryUserName;
        static Common::WStringLiteral const RegistryPassword;
        static Common::WStringLiteral const PasswordEncrypted;
        static Common::WStringLiteral const ComposeFileContent;
        static Common::WStringLiteral const RegistryCredential;
        // Deprecated
        static Common::WStringLiteral const RepositoryUserName;
        static Common::WStringLiteral const RepositoryPassword;
        static Common::WStringLiteral const RepositoryCredential;

        static Common::WStringLiteral const ComposeDeploymentStatus;

        //
        // Single Instance
        //
        static Common::WStringLiteral const deploymentName;
        static Common::WStringLiteral const applicationName;
        static Common::WStringLiteral const applicationUri;
        static Common::WStringLiteral const status;
        static Common::WStringLiteral const statusDetails;
        static Common::WStringLiteral const instanceCount;
        static Common::WStringLiteral const properties;
        static Common::WStringLiteral const containerRegistryServer;
        static Common::WStringLiteral const containerRegistryUserName;
        static Common::WStringLiteral const containerRegistryPassword;
        static Common::WStringLiteral const restartPolicy;
        static Common::WStringLiteral const volumes;
        static Common::WStringLiteral const image;
        static Common::WStringLiteral const command;
        static Common::WStringLiteral const ports;
        static Common::WStringLiteral const environmentVariables;
        static Common::WStringLiteral const instanceView;
        static Common::WStringLiteral const restartCount;
        static Common::WStringLiteral const currentState;
        static Common::WStringLiteral const previousState;
        static Common::WStringLiteral const state;
        static Common::WStringLiteral const exitCode;
        static Common::WStringLiteral const resources;
        static Common::WStringLiteral const resourcesRequests;
        static Common::WStringLiteral const resourcesLimits;
        static Common::WStringLiteral const volumeMounts;
        static Common::WStringLiteral const memoryInGB;
        static Common::WStringLiteral const cpu;
        static Common::WStringLiteral const mountPath;
        static Common::WStringLiteral const readOnly;
        static Common::WStringLiteral const imageRegistryCredentials;
        static Common::WStringLiteral const valueCamelCase;
        static Common::WStringLiteral const port;
        static Common::WStringLiteral const creationParameters;
        static Common::WStringLiteral const nameCamelCase;
        static Common::WStringLiteral const azureFile;
        static Common::WStringLiteral const shareName;
        static Common::WStringLiteral const storageAccountName;
        static Common::WStringLiteral const storageAccountKey;
        static Common::WStringLiteral const accountName;
        static Common::WStringLiteral const accountKey;
        static Common::WStringLiteral const sizeDisk;
        static Common::WStringLiteral const volumeName;
        static Common::WStringLiteral const volumeDescription;
        static Common::WStringLiteral const volumeDescriptionForImageBuilder;
        static Common::WStringLiteral const volumeParameters;
        static Common::WStringLiteral const osType;
        static Common::WStringLiteral const doNotPersistState;
        static Common::WStringLiteral const gatewayName;

        static Common::WStringLiteral const autoScalingPolicies;
        static Common::WStringLiteral const autoScalingName;
        static Common::WStringLiteral const autoScalingTrigger;
        static Common::WStringLiteral const autoScalingTriggerKind;
        static Common::WStringLiteral const autoScalingMetric;
        static Common::WStringLiteral const autoScalingMetricKind;
        static Common::WStringLiteral const autoScalingMetricName;
        static Common::WStringLiteral const autoScalingLowerLoadThreshold;
        static Common::WStringLiteral const autoScalingUpperLoadThreshold;
        static Common::WStringLiteral const autoScaleIntervalInSeconds;
        static Common::WStringLiteral const autoScalingMechanism;
        static Common::WStringLiteral const autoScalingMechanismKind;
        static Common::WStringLiteral const autoScalingMinInstanceCount;
        static Common::WStringLiteral const autoScalingMaxInstanceCount;
        static Common::WStringLiteral const autoScaleIncrement;

        static Common::WStringLiteral const RemoveServiceFabricRuntimeAccess;
        static Common::WStringLiteral const AzureFilePluginName;
        static Common::GlobalWString const ModelV2ReservedDnsNameCharacters;

        static Common::Global<std::vector<std::wstring>> const ValidNodeIdGeneratorVersionList;

        // Continuation Token
        //
        // A continuation token may contain more than one segment. For example, it may contain
        // and application name and an application version. This delimiter is used to denote
        // the separation between different parts of the continuation token.
        static std::wstring const ContinuationTokenParserDelimiter;
        static std::wstring const ContinuationTokenCreationFailedMessage;

        // Resource Governance
        //
        static Common::GlobalWString const SystemMetricNameCpuCores;
        static Common::GlobalWString const SystemMetricNameMemoryInMB;
        static uint const ResourceGovernanceCpuCorrectionFactor;

        // Container Network
        static Common::WStringLiteral const NetworkType;
        static Common::WStringLiteral const NetworkTypeCamelCase;
        static Common::WStringLiteral const NetworkName;
        static Common::WStringLiteral const NetworkNameCamelCase;
        static Common::WStringLiteral const NetworkAddressPrefix;
        static Common::WStringLiteral const NetworkAddressPrefixCamelCase;
        static Common::WStringLiteral const NetworkStatus;
        static Common::WStringLiteral const NetworkDescription;
        static Common::WStringLiteral const CodePackageVersion;
        static Common::WStringLiteral const ContainerAddress;
        static Common::WStringLiteral const ContainerId;
        static Common::WStringLiteral const ContainerNetworkPolicies;
        static Common::WStringLiteral const NetworkRef;
        static Common::WStringLiteral const EndpointBindings;
        static Common::WStringLiteral const EndpointRef;
        static Common::WStringLiteral const endpointRefsCamelCase;

        DECLARE_TEST_MUTABLE_CONSTANT(size_t, NamedPropertyMaxValueSize)
    };

    class StringConstants
    {
    public:
        static const Common::GlobalWString HttpVerb;
        static const Common::GlobalWString UriPath;
        static const Common::GlobalWString HttpContentType;
        static const Common::GlobalStringT HttpContentEncoding;
        static const Common::GlobalWString HttpRequestBody;
    };
}
