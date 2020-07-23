// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Placement.h"
#include "ServiceTypeDescription.h"
#include "NodeDescription.h"
#include "ServiceDescription.h"
#include "FailoverUnitDescription.h"
#include "BoundedSet.h"
#include "ViolationList.h"
#include "TraceHelpers.h"
#include "PLBSchedulerAction.h"
#include "ServiceType.h"
#include "Application.h"
#include "ApplicationLoad.h"
#include "DiagnosticsDataStructures.h"
#include "Reliability/Failover/common/PlbMovementIgnoredReasons.h"
#include "ServicePackageDescription.h"
#include "ServicePackageNode.h"
#include "PLBSchedulerActionType.h"
#include "FailoverUnitMovementType.h"
#include "SearchInsight.h"
#include "AutoScaleStatistics.h"
#include "RGStatistics.h"
#include "DefragStatistics.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
#define PLB_STRUCTURED_TRACE(traceName, traceId, traceLevel, ...) \
        traceName(taskCode, traceId, #traceName, Common::LogLevel::traceLevel, __VA_ARGS__)

#define PLB_QUERY_TRACE(traceName, traceId, traceType, traceLevel, ...) \
            traceName(Common::TraceTaskCodes::CRM, traceId, traceType, Common::LogLevel::traceLevel, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), __VA_ARGS__)

        // Exclude MasterCRM traces from customer info channel.
#define PLB_CUSTOMERINFO_TRACE(traceName, traceId, traceLevel, ...) \
        traceName(taskCode, traceId, #traceName, Common::LogLevel::traceLevel, Common::TraceChannelType::Debug, taskCode == Common::TraceTaskCodes::MCRM ? Common::TraceKeywords::Default : TRACE_KEYWORDS2(Default, CustomerInfo), __VA_ARGS__)

        class PLBEventSource
        {
        public:

            DECLARE_STRUCTURED_TRACE(UpdateFailoverUnit, Common::Guid, FailoverUnitDescription, int, bool);
            DECLARE_STRUCTURED_TRACE(UpdateFailoverUnitDeleted, Common::Guid);
            DECLARE_STRUCTURED_TRACE(UpdateNode, Federation::NodeId, NodeDescription);
            DECLARE_STRUCTURED_TRACE(UpdateServiceType, std::wstring, ServiceTypeDescription);
            DECLARE_STRUCTURED_TRACE(UpdateServiceTypeDeleted, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateService, std::wstring, ServiceDescription);
            DECLARE_STRUCTURED_TRACE(SkipServiceUpdate, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateServiceDeleted, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateLoadOrMoveCost, Common::Guid, int, std::wstring, uint);
            DECLARE_STRUCTURED_TRACE(IgnoreLoadOrMoveCostFUNotExist, Common::Guid);
            DECLARE_STRUCTURED_TRACE(IgnoreLoadInvalidMetric, Common::Guid, std::wstring, std::wstring, uint);
            DECLARE_STRUCTURED_TRACE(IgnoreLoadInvalidMetricOnNode, Common::Guid, std::wstring, std::wstring, uint, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(UpdateMovementEnabled, bool, bool);
            DECLARE_STRUCTURED_TRACE(PLBSkipObjectDisposed);
            DECLARE_STRUCTURED_TRACE(PLBSkipEmpty);
            DECLARE_STRUCTURED_TRACE(PLBSkipNoActionNeeded);
            DECLARE_STRUCTURED_TRACE(PLBStart, int64, int64, int64, int64, int64, int64, int64, int64, int);
            DECLARE_STRUCTURED_TRACE(PLBEnd, int, int64, int64);
            DECLARE_STRUCTURED_TRACE(PLBDomainStart, std::wstring, PLBSchedulerAction, int64, int64, std::wstring, int64, int64, int64, int64, double, size_t, size_t);
            DECLARE_STRUCTURED_TRACE(PLBDomainEnd, std::wstring, int, PLBSchedulerAction, double, double, int64);
            DECLARE_STRUCTURED_TRACE(PLBDomainInterrupted, std::wstring, PLBSchedulerAction, int64);
            DECLARE_STRUCTURED_TRACE(PLBDomainSkip, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(SchedulerActionSkip, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(PLBConstruct, int64, int64, int64, int64, int64);
            DECLARE_STRUCTURED_TRACE(PLBDispose);
            DECLARE_STRUCTURED_TRACE(Searcher, std::wstring);
            DECLARE_STRUCTURED_TRACE(PLBDump, uint16, std::wstring);
            DECLARE_STRUCTURED_TRACE(PLAGenerate);
            DECLARE_STRUCTURED_TRACE(PLAEmpty);
            DECLARE_STRUCTURED_TRACE(AffinityChainDetected, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(AddServiceDomain, std::wstring);
            DECLARE_STRUCTURED_TRACE(AddServiceToServiceDomain, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(DeleteServiceFromServiceDomain, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(DeleteServiceDomain, std::wstring);
            DECLARE_STRUCTURED_TRACE(MergeServiceDomain, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(PlacementConstraintParsingError, std::wstring);
            DECLARE_STRUCTURED_TRACE(PlacementConstraintEvaluationError, Federation::NodeId, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(PlacementRecord, uint16, std::wstring, std::vector<LoadBalancingDomainEntry>, std::wstring, std::vector<NodeEntry>, std::wstring, std::vector<ServiceEntry>, std::wstring, std::vector<PartitionEntry>, std::vector<ApplicationEntry>);
            DECLARE_STRUCTURED_TRACE(PlacementDump, std::wstring, Placement);
            DECLARE_STRUCTURED_TRACE(SearcherUnsuccessfulPlacement, size_t, size_t, BoundedSet<PlacementReplica const*>);
            DECLARE_STRUCTURED_TRACE(SearcherConstraintViolationFound, ViolationList);
            DECLARE_STRUCTURED_TRACE(SearcherConstraintViolationPartiallyCorrected, ViolationList);
            DECLARE_STRUCTURED_TRACE(SearcherConstraintViolationNotCorrected, ViolationList);
            DECLARE_STRUCTURED_TRACE(SearcherScoreImprovementNotAccepted, double, double, double, double);
            DECLARE_STRUCTURED_TRACE(NodeLoads, std::wstring, TraceLoadsVector<uint64, Node, ServiceDomainMetric, std::vector<Node>>);
            DECLARE_STRUCTURED_TRACE(PLBPeriodicalTrace, TraceVectorDescriptions<Node, NodeDescription>, TraceDescriptions<uint64, Application, ApplicationDescription, Uint64UnorderedMap<Application>>, TraceDescriptions<std::wstring, ServiceType, ServiceTypeDescription>, TraceDescriptions<int, const Service*, ServiceDescription>);
            DECLARE_STRUCTURED_TRACE(SimulatedAnnealingStatistics, Common::Guid, size_t, double, double, double, double);
            DECLARE_STRUCTURED_TRACE(SimulatedAnnealingStarted, size_t, std::wstring);
            DECLARE_STRUCTURED_TRACE(InsufficientResourcesDetected, std::wstring, std::wstring, int64, int64, int64, double, int64);
            DECLARE_STRUCTURED_TRACE(SplitServiceDomainStart, std::wstring, size_t);
            DECLARE_STRUCTURED_TRACE(SplitServiceDomainEnd, std::wstring, size_t);
            DECLARE_STRUCTURED_TRACE(SnapshotStart);
            DECLARE_STRUCTURED_TRACE(SnapshotEnd, int64);
            DECLARE_STRUCTURED_TRACE(SearchForUpgradeUnsucess, Common::Guid, std::wstring);
            DECLARE_STRUCTURED_TRACE(Scheduler, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(FailoverUnitNotFound, Common::Guid);
            DECLARE_STRUCTURED_TRACE(FailoverUnitNotStable, Common::Guid);
            DECLARE_STRUCTURED_TRACE(ReplicaInInvalidState, Common::Guid);
            DECLARE_STRUCTURED_TRACE(ReplicaNotFound, Common::Guid, std::wstring, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(CurrentPrimaryNotMatch, Federation::NodeId, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(SearchForUpgradeFlagIgnored, Common::Guid, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(SearchForUpgradePartitionIgnored, BoundedSet<Common::Guid>, std::wstring);
            DECLARE_STRUCTURED_TRACE(MetricDoesNotHaveThreshold, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(PLBNotReady);
            DECLARE_STRUCTURED_TRACE(SearcherUnsuccessfulSwapPrimary, BoundedSet<PlacementReplica const*>, bool);
            DECLARE_STRUCTURED_TRACE(UpdateMoveCost, Common::Guid, std::wstring, uint);
            DECLARE_STRUCTURED_TRACE(IgnoreInvalidMoveCost, Common::Guid, std::wstring, uint);
            DECLARE_STRUCTURED_TRACE(UpdateLoad, Common::Guid, std::wstring, std::wstring, uint);
            DECLARE_STRUCTURED_TRACE(UpdateLoadOnNode, Common::Guid, std::wstring, std::wstring, uint, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(UnplacedReplica, Common::Guid, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(OperationIgnored, Common::Guid, PlbMovementIgnoredReasons, Common::Guid);
            DECLARE_STRUCTURED_TRACE(OperationIgnoredQuery, Common::Guid, PlbMovementIgnoredReasons, Common::Guid);
            DECLARE_STRUCTURED_TRACE(PLBPrepareStart);
            DECLARE_STRUCTURED_TRACE(PLBPrepareEnd, int64);
            DECLARE_STRUCTURED_TRACE(ResetPartitionLoadStart, Common::Guid);
            DECLARE_STRUCTURED_TRACE(ResetPartitionLoadEnd, Common::Guid, Common::ErrorCode);
            DECLARE_STRUCTURED_TRACE(ConstraintKeyUndefined, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(ToggleVerboseServicePlacementHealthReporting, bool);
            DECLARE_STRUCTURED_TRACE(ResourceBalancerEvent, Common::Guid, Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateServiceDomainId, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnplacedReplicaDetails, Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnplacedPartition, Common::Guid, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnswappableUpgradePartition, Common::Guid, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnswappableUpgradePartitionDetails, Common::Guid, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(HealthReportingFailure, Common::ErrorCode);
            DECLARE_STRUCTURED_TRACE(UpdateApplication, std::wstring, ApplicationDescription);
            DECLARE_STRUCTURED_TRACE(DeleteApplication, std::wstring, ApplicationDescription);
            DECLARE_STRUCTURED_TRACE(AddApplicationToServiceDomain, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(ChangeApplicationServiceDomain, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(NotFoundInAppPartitionsMap, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(PLBRefreshTiming, uint64, uint64, uint64, uint64, uint64, uint64, uint64, uint64);
            DECLARE_STRUCTURED_TRACE(PLBProcessPendingUpdates, size_t, uint64);
            DECLARE_STRUCTURED_TRACE(UnfixableConstraintViolation, Common::Guid, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnfixableConstraintViolationDetails, Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(ResourceBalancerEventDiscarded, Common::Guid, PLBSchedulerAction, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(ApplicationLoads, std::wstring, TraceLoads<uint64, Application, ApplicationLoad, Uint64UnorderedMap<Application>, Uint64UnorderedMap<ApplicationLoad>>);
            DECLARE_STRUCTURED_TRACE(MoveReplicaOrInstance, std::wstring);
            DECLARE_STRUCTURED_TRACE(PrimaryNodeNotFound, Common::Guid);
            DECLARE_STRUCTURED_TRACE(PlacementReplicaDiffError, Common::Guid, int);
            DECLARE_STRUCTURED_TRACE(PlacementCreatorServiceNotFound, Common::Guid, std::wstring);
            DECLARE_STRUCTURED_TRACE(ServiceTypeNotFound, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(ServiceDomainNotFound, std::wstring);
            DECLARE_STRUCTURED_TRACE(InternalError, std::wstring);
            DECLARE_STRUCTURED_TRACE(InsufficientResourceForApplicationUpdate, std::wstring, ApplicationDescription, std::wstring, int64, int64, int64, int64);
            DECLARE_STRUCTURED_TRACE(SchedulerAction, std::wstring, Common::Guid, std::wstring, ServiceDomainSchedulerStageDiagnostics);
            DECLARE_STRUCTURED_TRACE(Decision, Common::Guid, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(Operation, Common::Guid, Common::Guid, PLBSchedulerActionType::Trace, FailoverUnitMovementType::Trace, std::wstring, Federation::NodeId, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(OperationQuery, Common::Guid, Common::Guid, PLBSchedulerActionType::Trace, FailoverUnitMovementType::Trace, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(DecisionToken1, uint16, Common::Guid, int32, bool, bool, bool, bool, bool, bool);
            DECLARE_STRUCTURED_TRACE(DecisionToken2, uint16, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool);
            DECLARE_STRUCTURED_TRACE(DecisionToken0, uint16, DecisionToken);
            DECLARE_STRUCTURED_TRACE(DecisionToken, DecisionToken);
            DECLARE_STRUCTURED_TRACE(OperationDiscarded, Common::Guid, Common::Guid, PLBSchedulerAction, std::wstring, std::wstring, Federation::NodeId, Federation::NodeId);
            DECLARE_STRUCTURED_TRACE(ApplicationNotFound, std::wstring, uint64, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnplacedReplicaInUpgrade, Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UnplacedReplicaInUpgradeDetails, Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateUpgradeCompletedUDs, std::wstring, bool, std::wstring);
            DECLARE_STRUCTURED_TRACE(DefragMetricInfoStart, TraceDefragMetricInfo);
            DECLARE_STRUCTURED_TRACE(BalancingMetricInfoStart, TraceAllBalancingMetricInfo);
            DECLARE_STRUCTURED_TRACE(DefragMetricInfoEnd, TraceDefragMetricInfo);
            DECLARE_STRUCTURED_TRACE(BalancingMetricInfoEnd, TraceAllBalancingMetricInfo);
            DECLARE_STRUCTURED_TRACE(InconsistentReplicaLoad, Federation::NodeId, Common::Guid, std::wstring, std::wstring, std::wstring, uint, uint);
            DECLARE_STRUCTURED_TRACE(ConstraintFixDiagnosticsFailed, Common::Guid);
            DECLARE_STRUCTURED_TRACE(UpdateServicePackage, std::wstring, ServicePackageDescription);
            DECLARE_STRUCTURED_TRACE(DeleteServicePackage, std::wstring);
            DECLARE_STRUCTURED_TRACE(ServicePackageInsufficientResourceUpgrade, Federation::NodeId, std::wstring, int64, int64, int64, ServicePackageDescription, ServicePackageNode);
            DECLARE_STRUCTURED_TRACE(ApplicationInRGChange, ApplicationDescription);
            DECLARE_STRUCTURED_TRACE(ParentServiceMultipleParitions, ServiceDescription);
            DECLARE_STRUCTURED_TRACE(MergingApplicationLoad, std::wstring, uint64, std::wstring);
            DECLARE_STRUCTURED_TRACE(TrackingProposedMovementsEnd, uint64);
            DECLARE_STRUCTURED_TRACE(PLBSearchInsight, SearchInsight);
            DECLARE_STRUCTURED_TRACE(SearcherBatchPlacement, size_t, int);
            DECLARE_STRUCTURED_TRACE(DetailedSimulatedAnnealingStatistic, size_t, std::wstring);
            DECLARE_STRUCTURED_TRACE(ResourceGovernanceStatistics, RGStatistics);
            DECLARE_STRUCTURED_TRACE(AutoScalingStatistics, AutoScaleStatistics);
            DECLARE_STRUCTURED_TRACE(AutoScalingRepartitionFailed, uint64_t, Common::ErrorCode);
            DECLARE_STRUCTURED_TRACE(AutoScaler, std::wstring);
            DECLARE_STRUCTURED_TRACE(DefragmentationStatistics, DefragStatistics);
            DECLARE_STRUCTURED_TRACE(AvailableImagesFromNode, std::wstring);
            DECLARE_STRUCTURED_TRACE(UpdateNodeImages, std::wstring);
            DECLARE_STRUCTURED_TRACE(InvalidAutoScaleMinCount, std::wstring);

            PLBEventSource(Common::TraceTaskCodes::Enum taskCode) :
                PLB_STRUCTURED_TRACE(UpdateFailoverUnit, 7, Info, "Updating failover unit with {1} actualReplicaDiff:{2} interruptBalancing:{3}", "id", "fuDescription", "actualReplicaDiff", "interrupt"),
                PLB_STRUCTURED_TRACE(UpdateFailoverUnitDeleted, 8, Info, "Deleting failover unit {0}", "id"),
                PLB_STRUCTURED_TRACE(UpdateNode, 9, Info, "Updating node with {1}", "id", "nodeDescription"),
                PLB_STRUCTURED_TRACE(UpdateServiceType, 15, Info, "Updating service type with {1}", "id", "serviceTypeDescription"),
                PLB_STRUCTURED_TRACE(UpdateServiceTypeDeleted, 16, Info, "Deleting service type {0}", "id"),
                PLB_STRUCTURED_TRACE(UpdateService, 10, Info, "Updating service with {1}", "id", "serviceDescription"),
                PLB_STRUCTURED_TRACE(SkipServiceUpdate, 46, Info, "Skip service update for {0} because {1}", "id", "reason"),
                PLB_STRUCTURED_TRACE(UpdateServiceDeleted, 11, Info, "Deleting service {0}", "id"),
                PLB_STRUCTURED_TRACE(UpdateLoadOrMoveCost, 12, Info, "Updating load or move cost for role {1} with {2}:{3}", "id", "role", "metricName", "value"),
                PLB_STRUCTURED_TRACE(IgnoreLoadOrMoveCostFUNotExist, 13, Info, "Ignoring a load or move cost report because failover unit id does not exist", "id"),
                PLB_STRUCTURED_TRACE(IgnoreLoadInvalidMetric, 14, Warning, "Ignoring a load report for role {1} with {2}:{3} because metric name doesn't exist or it is a built-in metric", "id", "role", "metricName", "value"),
                PLB_STRUCTURED_TRACE(UpdateMovementEnabled, 17, Info, "Updating ConstraintCheckEnabled to {0} BalancingEnabled to {1}", "constraintCheckEnabled", "balancingEnabled"),
                PLB_STRUCTURED_TRACE(PLBSkipObjectDisposed, 19, Info, "Skip PLB because PLB object disposed"),
                PLB_STRUCTURED_TRACE(PLBSkipEmpty, 20, Info, "Skip PLB because there is no up node, service or failover unit"),
                PLB_STRUCTURED_TRACE(PLBSkipNoActionNeeded, 24, Info, "Skip PLB because no service domain needs to take action"),
                PLB_STRUCTURED_TRACE(PLBStart, 26, Info, "PLB started with nodeCount:{0} serviceCount:{1} failoverUnitCount:{2}(inTransition:{3}) existingReplicaCount:{4}(movable:{5}) partitionsWithNewReplica:{6} partitionsWithInstanceOnEveryNode:{7} randomSeed:{8}", "nodeCount", "serviceCount", "failoverUnitCount", "inTransitionFailoverUnitCount", "existingReplicaCount", "movableReplicaCount", "partitionsWithNewReplicaCount", "partitionsWithInstOnEveryNodeCount", "randomSeed"),
                PLB_STRUCTURED_TRACE(PLBEnd, 27, Info, "PLB generated {0} actions, {1} ms used ({2} ms used for engine run.)", "numberOfMovements", "timeSpent", "engineTimeSpent"),
                PLB_STRUCTURED_TRACE(PLBDomainStart, 32, Info, "PLB for domain {0} running {1} with nodeCount:{2} serviceCount:{3} imbalancedServices:{4} failoverUnitCount:{5} existingReplicaCount:{6} toBeCreatedReplicaCount:{7} partitionsInUpgrade:{8} originalDev:{9} quorumBasedServicesCount:{10} quorumBasedPartitionsCount:{11}", "id", "action", "nodeCount", "serviceCount", "imbalancedServices", "failoverUnitCount", "existingReplicaCount", "toBeCreatedReplicaCount", "partitionsInUpgrade", "originalDeviation", "quorumBasedServicesCount", "quorumBasedPartitionsCount"),
                PLB_STRUCTURED_TRACE(PLBDomainEnd, 33, Info, "PLB for domain {0} generated {1} {2}, domainScoreBefore:{3}, domainScoreAfter:{4}, {5} ms used", "id", "numberOfMovements", "action", "originalDeviation", "newDeviation", "timeSpent"),
                PLB_STRUCTURED_TRACE(PLBDomainInterrupted, 28, Info, "PLB for domain {0} running {1} was interrupted, {2} ms used", "id", "action", "timeSpent"),
                PLB_STRUCTURED_TRACE(PLBDomainSkip, 23, Info, "PLB for domain {0} is skipped because {1}", "id", "reason"),
                PLB_STRUCTURED_TRACE(PLBConstruct, 29, Info, "PLB constucted with nodeCount:{0}, serviceTypeCount:{1}, serviceCount:{2}, failoverUnitCount:{3}, loadMetricCount:{4}", "nodeCount", "serviceTypeCount", "serviceCount", "failoverUnitCount", "loadMetricCount"),
                PLB_STRUCTURED_TRACE(PLBDispose, 30, Info, "PLB disposed"),
                PLB_STRUCTURED_TRACE(Searcher, 31, Info, "{0}", "event"),
                PLB_STRUCTURED_TRACE(PLBDump, 34, Info, "{1}\r\n", "contextSequenceId", "text"),
                PLB_STRUCTURED_TRACE(PLAGenerate, 35, Info, "Generating placement advisor"),
                PLB_STRUCTURED_TRACE(PLAEmpty, 36, Info, "Generating placement advisor empty"),
                PLB_STRUCTURED_TRACE(AffinityChainDetected, 37, Error, "Adding the service affinity relationship {0} -> {1} would create an affinity chain with length greater than 1, which is not supported", "fromService", "toService"),
                PLB_STRUCTURED_TRACE(AddServiceDomain, 38, Info, "Add service domain {0}", "id"),
                PLB_STRUCTURED_TRACE(AddServiceToServiceDomain, 39, Info, "Add service {1} to service domain {0}", "id", "serviceName"),
                PLB_STRUCTURED_TRACE(DeleteServiceFromServiceDomain, 40, Info, "Delete service {1} from service domain {0}", "id", "serviceName"),
                PLB_STRUCTURED_TRACE(DeleteServiceDomain, 41, Info, "Delete service domain {0}", "id"),
                PLB_STRUCTURED_TRACE(MergeServiceDomain, 42, Info, "Merge service domain {1} with service domain {0}", "id", "anotherServiceDomain"),
                PLB_STRUCTURED_TRACE(PlacementConstraintParsingError, 43, Warning, "Placement constraint expression parsing error: {0}", "placementConstraint"),
                PLB_STRUCTURED_TRACE(PlacementConstraintEvaluationError, 51, Warning, "Placement constraint evaluation error on node {0} with expression {1}: {2}", "nodeId", "placementConstraint", "reason"),
                PLB_STRUCTURED_TRACE(PlacementRecord, 44, Info, "{1}\r\n{2}\r\n{3}\r\n{4}\r\n{5}\r\n{6}\r\n{7}\r\n{8}\r\n{9}", "contextSequenceId", "lbDomainEntryDescription", "lbDomains", "nodeEntryDescription", "nodes", "serviceEntryDescription", "services", "partitionEntryDescription", "partitions", "applications"),
                PLB_STRUCTURED_TRACE(PlacementDump, 45, Noise, "#Placement object dump, you can copy/paste into LBSimulator for debugging\r\n{1}", "id", "placement"),
                PLB_STRUCTURED_TRACE(SearcherUnsuccessfulPlacement, 47, Info, "Unable to place {0} out of {1} new replicas. Unplaced replicas include: {2}", "unsuccessfulReplicaCount", "newReplicaCount", "unplacedReplicas"),
                PLB_STRUCTURED_TRACE(SearcherConstraintViolationFound, 55, Info, "Constraint violations found: {0}", "violations"),
                PLB_STRUCTURED_TRACE(SearcherConstraintViolationPartiallyCorrected, 48, Info, "Constraint violations are partially corrected. Remaining violations: {0}", "violations"),
                PLB_STRUCTURED_TRACE(SearcherConstraintViolationNotCorrected, 49, Info, "Constraint violations are not corrected. Remaining violations: {0}", "violations"),
                PLB_STRUCTURED_TRACE(SearcherScoreImprovementNotAccepted, 50, Info, "Score improvement ({0}) from old AvgStdDev ({1}) to new AvgStdDev ({2}) is less than ScoreImprovementThreshold ({3}); not accepted.", "scoreImprovement", "oldStdDev", "newStdDev", "threshold"),
                PLB_STRUCTURED_TRACE(NodeLoads, 52, Info, "{1}", "id", "nodeLoads"),
                PLB_STRUCTURED_TRACE(PLBPeriodicalTrace, 56, Info, "{0}\r\n{1}\r\n{2}\r\n{3}", "nodes", "applications", "serviceTypes", "services"),
                PLB_STRUCTURED_TRACE(SimulatedAnnealingStatistics, 53, Noise, "Simulated annealing statistics: {1} {2} {3} {4} {5}", "id", "iteration", "temperature", "avgStdDev", "score", "bestScore"),
                PLB_STRUCTURED_TRACE(SimulatedAnnealingStarted, 54, Noise, "Simulated annealing started with {0} solutions: {1}", "solutionCount", "solutionIds"),
                PLB_STRUCTURED_TRACE(InsufficientResourcesDetected, 57, Warning, "Insufficient resources when adding service {0} for metric {1}: Requested {2}, Available {3}, Reserved{4} BufferPercentage {5}, Capacity {6}", "service", "metric", "requestedResource", "availableResource", "reserved", "bufferPercentage", "totalCapacity"),
                PLB_STRUCTURED_TRACE(SplitServiceDomainStart, 58, Info, "Split Service Domain Start for Domain: {0}, Total Service Domain Count: {1}", "id", "count"),
                PLB_STRUCTURED_TRACE(SplitServiceDomainEnd, 59, Info, "Split Service Domain End for Domain: {0}, Total Service Domain Count: {1}", "id", "count"),
                PLB_STRUCTURED_TRACE(SnapshotStart, 60, Info, "Taking snapshot begin."),
                PLB_STRUCTURED_TRACE(SnapshotEnd, 61, Info, "Taking snapshot end. Total ms: {0}", "timeSpent"),
                PLB_STRUCTURED_TRACE(SearchForUpgradeUnsucess, 62, Warning, "SearchForUpgrade: Did not find a solution for partition {0} with flag {1}", "partition", "flag"),
                PLB_STRUCTURED_TRACE(Scheduler, 63, Info, "{1}", "id", "event"),
                PLB_STRUCTURED_TRACE(FailoverUnitNotFound, 64, Warning, "FailoverUnit {0} not found", "failoverUnitId"),
                PLB_STRUCTURED_TRACE(FailoverUnitNotStable, 65, Warning, "FailoverUnit {0} is not stable", "failoverUnitId"),
                PLB_STRUCTURED_TRACE(ReplicaInInvalidState, 66, Warning, "Replica to be promoted or moved is in invalid state - {0}", "failoverUnitId"),
                PLB_STRUCTURED_TRACE(ReplicaNotFound, 67, Warning, "Replica {0} of service {1} not found on specified node {2}", "failoverUnitId", "serviceName", "nodeId"),
                PLB_STRUCTURED_TRACE(CurrentPrimaryNotMatch, 68, Warning, "currentPrimary does not match actual {0}, expected {1}", "replicaNodeId", "currentPrimary"),
                PLB_STRUCTURED_TRACE(SearchForUpgradeFlagIgnored, 69, Info, "SearchForUpgrade: Flag is ignored for partition {0} with flag {1} because {2}", "partition", "flag", "reason"),
                PLB_STRUCTURED_TRACE(SearchForUpgradePartitionIgnored, 70, Info, "SearchForUpgrade: Search is ignored for partitions {0} because {1}", "partition", "reason"),
                PLB_STRUCTURED_TRACE(MetricDoesNotHaveThreshold, 71, Info, "Reported metric {0} of service {1} does not have activity or balancing threshold", "metricName", "serviceName"),
                PLB_STRUCTURED_TRACE(PLBNotReady, 72, Info, "PLB is not ready. Please retry."),
                PLB_STRUCTURED_TRACE(SearcherUnsuccessfulSwapPrimary, 73, Info, "Unable to find swap solution for {0}, relaxCapacity {1}", "invalidPartitions", "relaxed"),
                PLB_STRUCTURED_TRACE(UpdateMoveCost, 74, Info, "Updating move cost for role {1} with {2}", "id", "role", "value"),
                PLB_STRUCTURED_TRACE(IgnoreInvalidMoveCost, 75, Warning, "Ignoring a move cost report for role {1} with value {2} because value is not valid move cost", "id", "role", "value"),
                PLB_STRUCTURED_TRACE(UpdateLoad, 76, Info, "Updating load for role {1} with {2}:{3}", "id", "role", "metricName", "value"),
                PLB_STRUCTURED_TRACE(UnplacedReplica, 77, Info, "{1} {2} Partition {0} could not be placed:{3}", "id", "ServiceName", "Role", "Reasons"),
                PLB_CUSTOMERINFO_TRACE(OperationIgnored, 78, Info, "An Operation on FailoverUnit {0} was ignored because of reason '{1}'. Decision Id: {2}.", "FailoverUnitId", "Reason", "DecisionId"),
                PLB_STRUCTURED_TRACE(PLBPrepareStart, 79, Info, "PLB Prepare start, Processing Pending Update and taking snapshot."),
                PLB_STRUCTURED_TRACE(PLBPrepareEnd, 80, Info, "PLB Prepare end. Total ms: {0}", "timeSpent"),
                PLB_STRUCTURED_TRACE(ResetPartitionLoadStart, 81, Info, "Reset load begin for partition: {0}", "PartitionId"),
                PLB_STRUCTURED_TRACE(ResetPartitionLoadEnd, 82, Info, "Reset load end for partition: {0}, error: {1}", "PartitionId", "ErrorCode"),
                PLB_STRUCTURED_TRACE(ConstraintKeyUndefined, 83, Warning, "Creation of Service: {0} failed because there were undefined constraint keys in the PlacementConstraint Expression: {1}", "serviceName", "placementConstraints"),
                PLB_STRUCTURED_TRACE(SchedulerActionSkip, 84, Info, "Action {1} for domain {0} is skipped because {2}", "id", "action", "reason"),
                PLB_STRUCTURED_TRACE(IgnoreLoadInvalidMetricOnNode, 85, Warning, "Ignoring a load report for role {1} with {2}:{3} on node {4} because metric name doesn't exist or it is a built-in metric", "id", "role", "metricName", "value", "nodeId"),
                PLB_STRUCTURED_TRACE(UpdateLoadOnNode, 86, Info, "Updating load for role {1} with {2}:{3} on node {4}", "id", "role", "metricName", "value", "nodeId"),
                PLB_STRUCTURED_TRACE(ToggleVerboseServicePlacementHealthReporting, 87, Info, "Verbose Health Reporting for placement is now enabled: {0}", "enabled"),
                PLB_CUSTOMERINFO_TRACE(ResourceBalancerEvent, 88, Info, "Completed the {2} phase, with DecisionId {1}, and issued the action -- {3}, on Service -- {4}, Role -- {7}, from (if relevant) SourceNode -- {5} to TargetNode -- {6}.", "id", "decisionId", "SchedulerPhase", "Action", "ServiceName", "SourceNode", "TargetNode", "Role"),
                PLB_STRUCTURED_TRACE(UpdateServiceDomainId, 89, Info, "Update service domain id {0} end. new id {1}", "OldId", "NewId"),
                PLB_STRUCTURED_TRACE(UnplacedReplicaDetails, 90, Info, "{1} {2} Partition {0} could not be placed:{3} {4}", "id", "ServiceName", "Role", "Reasons", "Nodes"),
                PLB_STRUCTURED_TRACE(UnplacedPartition, 91, Info, "PLB cannot place all replicas for service {1} partition {0}: {2}", "id", "ServiceName", "Reason"),
                PLB_STRUCTURED_TRACE(UnswappableUpgradePartition, 92, Info, "{1} Partition {0} could not be placed:{2}", "id", "ServiceName", "Reasons"),
                PLB_STRUCTURED_TRACE(UnswappableUpgradePartitionDetails, 93, Info, "{1} Partition {0} could not be placed:{2} {3}", "id", "ServiceName", "Reasons", "Nodes"),
                PLB_STRUCTURED_TRACE(HealthReportingFailure, 94, Info, "The Resource Balancer failed while trying to report {0}", "AddHealthError"),
                PLB_STRUCTURED_TRACE(UpdateApplication, 95, Info, "Updating application with {1}", "id", "applicationDescription"),
                PLB_STRUCTURED_TRACE(DeleteApplication, 96, Info, "Deleting application with {1}", "id", "applicationDescription"),
                PLB_STRUCTURED_TRACE(AddApplicationToServiceDomain, 97, Info, "Add application {0} to service domain {1}", "applicationName", "id"),
                PLB_STRUCTURED_TRACE(ChangeApplicationServiceDomain, 98, Info, "Change application {0} service domain from {1} to {2}", "applicationName", "domain1", "domain2"),
                PLB_STRUCTURED_TRACE(NotFoundInAppPartitionsMap, 99, Warning, "{0}: application {1} is NOT in application partitions map", "reason", "applicationName"),
                PLB_STRUCTURED_TRACE(PLBRefreshTiming, 100, Info, "Timing for phases of refresh: total={0} processingUpdates={1} beginRefresh={2} endRefresh={3} snapshot={4} engine={5} autoScaling={6} remainder={7}", "Total", "PendingUpdates", "BeginRefresh", "EndRefresh", "Snapshot", "Engine", "AutoScaling", "Remainder"),
                PLB_STRUCTURED_TRACE(PLBProcessPendingUpdates, 101, Info, "PLB processed {0} updates in {1} ms", "NumUpdates", "Time"),
                PLB_STRUCTURED_TRACE(UnfixableConstraintViolation, 102, Info, "{1} {2} Partition {0} constraint violation could not be fixed:{3}", "id", "ServiceName", "Role", "Reasons"),
                PLB_STRUCTURED_TRACE(UnfixableConstraintViolationDetails, 103, Info, "{1} {2} Partition {0} constraint violation could not be fixed:{3} {4}", "id", "ServiceName", "Role", "Reasons", "Nodes"),
                PLB_CUSTOMERINFO_TRACE(ResourceBalancerEventDiscarded, 104, Info, "Completed the {1} phase and discarded the action -- {2}, on Service -- {3}, Role -- {6}, from (if relevant) SourceNode -- {4} to TargetNode -- {5}.", "id", "SchedulerPhase", "Action", "ServiceName", "SourceNode", "TargetNode", "Role"),
                PLB_STRUCTURED_TRACE(ApplicationLoads, 105, Info, "{1}", "id", "applicationLoads"),
                PLB_STRUCTURED_TRACE(MoveReplicaOrInstance, 106, Info, "{0}", "MoveReplicaOrInstanceDiagnostics"),
                PLB_STRUCTURED_TRACE(PrimaryNodeNotFound, 107, Warning, "Primary Replica's Node not found when reporting load on partition.", "id"),
                PLB_STRUCTURED_TRACE(PlacementReplicaDiffError, 108, Error, "PlacementCreator has calculated negative replica difference -- partition: {0} replicaDiff:{1}", "partitionId", "replicaDiff"),
                PLB_STRUCTURED_TRACE(PlacementCreatorServiceNotFound, 109, Error, "PlacementCreator does not have service entry for service {1} of partition {0}", "partitionId", "serviceName"),
                PLB_STRUCTURED_TRACE(ServiceTypeNotFound, 110, Error, "ServiceType {1} not found for service {0}", "serviceName", "serviceTypeName"),
                PLB_STRUCTURED_TRACE(ServiceDomainNotFound, 111, Error, "Service domain not found for service {0}. Update is ignored.", "serviceName"),
                PLB_STRUCTURED_TRACE(InternalError, 112, Error, "An internal error has occurred in CRM: {0}", "errorMessage"),
                PLB_STRUCTURED_TRACE(InsufficientResourceForApplicationUpdate, 113, Error, "Insufficient cluster resource when updating application {1} - Metric {2}, New Reservation: {3}, Reservation Increase: {4}, Remaining Capacity: {5}, Remaining Reserved Load: {6}", "id", "appDescription", "metricName", "newReservation", "reservationIncrease", "remainingCapacity", "remainingReservation"),
                PLB_STRUCTURED_TRACE(SchedulerAction, 114, Info, "ServiceDomainId: {0}\r\nDecisionId: {1}\r\nAffects Services with Metrics: {2}\r\nReasons for Decision: {3}\r\n", "id", "decisionId", "metrics", "reasons"),
                PLB_CUSTOMERINFO_TRACE(Decision, 115, Info, "DecisionId: {0}\r\nAffects Services with Metrics: {1}\r\nReasons for Decision: {2}\r\n", "id", "metrics", "reasons"),
                PLB_CUSTOMERINFO_TRACE(Operation, 116, Info, "Operation Details:\r\nPhase: {2} \r\nAction: {3} \r\nService: {4} \r\nDecisionId: {1}\r\nPartitionId: {0} \r\nSourceNode: {5}\r\nTargetNode: {6}", "id", "decisionId", "SchedulerPhase", "Action", "ServiceName", "SourceNode", "TargetNode"),
                PLB_STRUCTURED_TRACE(DecisionToken1, 117, Info, "DecisionId: {1}\r\nTokenValue: {2}\r\nClientAPICall {3} \r\nSkip {4}\r\nNoneExpired {5}\r\nPlacement {6}\r\nBalancing {7}\r\nConstraintCheck {8}", "contextSequenceId", "DecisionId", "TokenValue", "ClientAPICall", "Skip", "NoneExpired", "Placement", "Balancing", "ConstraintCheck"),
                PLB_STRUCTURED_TRACE(DecisionToken2, 118, Info, "\r\nReplicaExclusionStatic {1}\r\nReplicaExclusionDynamic {2}\r\nPlacementConstraint {3}\r\nNodeCapacity {4}\r\nAffinity {5}\r\nFaultDomain {6}\r\nUpgradeDomain {7}\r\nPreferredLocation {8}\r\nScaleoutCount {9}\r\nApplicationCapacity {10}", "contextSequenceId", "ReplicaExclusionStatic", "ReplicaExclusionDynamic", "PlacementConstraint", "NodeCapacity", "Affinity", "FaultDomain", "UpgradeDomain", "PreferredLocation", "ScaleoutCount", "ApplicationCapacity"),
                PLB_STRUCTURED_TRACE(DecisionToken0, 119, Info, "Decision:\r\n{1}", "contextSequenceId", "DecisionToken"),
                PLB_STRUCTURED_TRACE(DecisionToken, 120, Info, "{0}", "DecisionToken"),
                PLB_CUSTOMERINFO_TRACE(OperationDiscarded, 121, Info, "Operation Discarded:\r\nPhase: {2} \r\nAction: {3} \r\nService: {4} \r\nDecisionId: {1} \r\nPartitionId: {0} \r\nSourceNode: {5} \r\nTargetNode: {6}", "id", "decisionId", "SchedulerPhase", "Action", "ServiceName", "SourceNode", "TargetNode"),
                PLB_STRUCTURED_TRACE(ApplicationNotFound, 122, Error, "Application {0} with ID: {1} not found for service {2}", "applicationName", "applicationId", "serviceName"),
                PLB_STRUCTURED_TRACE(UnplacedReplicaInUpgrade, 123, Info, "{1} {2} Partition {0} that was evaluated together with partitions {3} could not be placed:{4}", "id", "ServiceName", "Role", "Partitions", "Reasons"),
                PLB_STRUCTURED_TRACE(UnplacedReplicaInUpgradeDetails, 124, Info, "{1} {2} Partition {0} that was evaluated with partitions {3} could not be placed:{4} {5}", "id", "ServiceName", "Role", "Partitions", "Reasons", "Nodes"),
                PLB_STRUCTURED_TRACE(UpdateUpgradeCompletedUDs, 125, Info, "Updating {0} upgraded UDs: IsInProgress: {1}; completed UDs: {2}", "UpgradeType", "inProgress", "UDs"),
                PLB_STRUCTURED_TRACE(DefragMetricInfoStart, 126, Info, "{0}", "defragMetricInfo"),
                PLB_STRUCTURED_TRACE(BalancingMetricInfoStart, 127, Info, "{0}", "balancingMetricInfo"),
                PLB_STRUCTURED_TRACE(DefragMetricInfoEnd, 128, Info, "{0}", "defragMetricInfo"),
                PLB_STRUCTURED_TRACE(BalancingMetricInfoEnd, 129, Info, "{0}", "balancingMetricInfo"),
                PLB_STRUCTURED_TRACE(InconsistentReplicaLoad, 130, Warning, "Replica on node {0} from partition {1} with role {2} and state {3} have inconsistent load for metric {4}. Previous added load: {5}, current delete load:{6}.", "nodeId", "id", "replicaRole", "replicaState", "metric", "addedLoad", "deletedLoad"),
                PLB_STRUCTURED_TRACE(ConstraintFixDiagnosticsFailed, 131, Warning, "{0}", "PartitionId"),
                PLB_STRUCTURED_TRACE(UpdateServicePackage, 132, Info, "Updating Service Package {0} with {1}", "Name", "ServicePackageDescription"),
                PLB_STRUCTURED_TRACE(DeleteServicePackage, 133, Info, "Deleting Service Package {0}", "Name"),
                PLB_STRUCTURED_TRACE(ServicePackageInsufficientResourceUpgrade, 134, Info, "Insufficient resources on node {0} for metric {1} with load/should disappear/capacity {2}/{3}/{4}, service package {5} with replica counts {6}", "nodeId", "metricName", "load", "shouldDisappearLoad", "capacity", "ServicePackageDescription", "ServicePackageNode"),
                PLB_STRUCTURED_TRACE(ApplicationInRGChange, 135, Info, "Application {0} scheduled to check for resources", "appDescription"),
                PLB_STRUCTURED_TRACE(ParentServiceMultipleParitions, 136, Error, "Parent {0} has multiple partitions", "parent"),
                PLB_STRUCTURED_TRACE(MergingApplicationLoad, 137, Error, "Error while merging {0} in ApplicationLoad (applicationId = {1}) for metric {2} which is contained in both domains.", "tableName", "applicationId", "metricName"),
                PLB_STRUCTURED_TRACE(TrackingProposedMovementsEnd, 138, Info, "TracingTime={0}", "TracingTime"),
                PLB_STRUCTURED_TRACE(PLBSearchInsight, 139, Info, "Search insight: \r\n {0}", "searchInsight"),
                PLB_STRUCTURED_TRACE(SearcherBatchPlacement, 140, Info, "Batch placement would be run; New replica count in placement {0} is greater than configed BatchPlacementReplicaCount {1}", "ReplicaCount", "ConfigCount"),
                PLB_STRUCTURED_TRACE(DetailedSimulatedAnnealingStatistic, 141, Info, "Detailed simmulated annealing statistic: number of solutions {0}.\r\nTotal successful tries: {1}", "solutionsNumber", "successfulTriesPerSolution"),
                PLB_STRUCTURED_TRACE(ResourceGovernanceStatistics, 142, Info, "{0}", "rgStatistics"),
                PLB_QUERY_TRACE(OperationQuery, 143, "_Partitions_Operation", Info, "Operation Details: Phase: {2} Action: {3} Service: {4} DecisionId: {1} PartitionId: {0} SourceNode: {5} TargetNode: {6}", "id", "decisionId", "SchedulerPhase", "Action", "ServiceName", "SourceNode", "TargetNode"),
                PLB_QUERY_TRACE(OperationIgnoredQuery, 144, "_Partitions_OperationIgnored", Info, "An Operation on FailoverUnit {0} was ignored because of reason '{1}'. Decision Id: {2}.", "FailoverUnitId", "Reason", "DecisionId"),
                PLB_STRUCTURED_TRACE(AutoScalingStatistics, 145, Info, "{0}", "autoscaleStatistics"),
                PLB_STRUCTURED_TRACE(AutoScalingRepartitionFailed, 146, Warning, "Repartitioning of service with id{0} failed with error {1}", "serviceId", "error"),
                PLB_STRUCTURED_TRACE(AutoScaler, 147, Info, "{0}", "message"),
                PLB_STRUCTURED_TRACE(DefragmentationStatistics, 148, Info, "{0}", "defragStatistics"),
                PLB_STRUCTURED_TRACE(AvailableImagesFromNode, 149, Info, "{0}", "message"),
                PLB_STRUCTURED_TRACE(UpdateNodeImages, 150, Info, "{0}", "message"),
                PLB_STRUCTURED_TRACE(InvalidAutoScaleMinCount, 151, Error, "{0} : Autoscaling policy is not allowed with Min count zero", "name")
            {

            }
        };
    }
}
