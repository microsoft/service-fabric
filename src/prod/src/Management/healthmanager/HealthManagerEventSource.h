// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
#define END_HM_STRUCTURED_TRACES \
        dummyMember() \
        { } \
        bool dummyMember; \

#define DECLARE_HM_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define HM_TRACE(trace_name, trace_id, trace_level, ...) \
        STRUCTURED_TRACE(trace_name, HealthManager, trace_id, trace_level, __VA_ARGS__),

        class HealthManagerEventSource
        {
        public:
        HealthManagerEventSource() :
            // Replica Life-cycle
            //
            HM_TRACE( ReplicaCtor, 10, Info, "{0}: node={1}, refAddr={2}", "prId", "node", "refAddr")
            HM_TRACE( ReplicaDtor, 11, Info, "{0}: refAddr={1}", "prId", "refAddr")
            HM_TRACE( ReplicaOpen, 12, Info, "{0}: node={1}", "prId", "node" )
            HM_TRACE( ReplicaChangeRole, 13, Info, "{0}: new role = {1}", "prId", "role" )
            HM_TRACE( ReplicaClose, 14, Info, "{0}: close", "prId" )
            HM_TRACE( ReplicaAbort, 15, Info, "{0}: abort", "prId" )
            HM_TRACE( ReplicaCloseCompleted, 16, Info, "{0}: close completed with {1}", "prId", "error" )

            // General Request Processing
            //
            HM_TRACE( InvalidMessage, 20, Info, "{0}: ({1}) message {2}: MISSING {3}", "prId", "action", "messageId", "reason" )
            HM_TRACE( InvalidMessage2, 21, Info, "{0}+{1}: ({2}) message {3}: MISSING {4}", "prId", "activityId", "action", "messageId", "missingHeader" )
            HM_TRACE( InvalidActor, 22, Info, "{0}+{1}: ({2}) message {3}: actor {4} not supported", "prId", "activityId", "action", "messageId", "actor" )
            HM_TRACE( RequestReceived, 23, Noise, "{0}+{1}: received ({2}) message {3}", "prId", "activityId", "action", "messageId")
            HM_TRACE( RequestRejected, 24, Info, "{0}+{1}: reject message {2}, error = {3}", "prId", "activityId", "messageId", "error" )
            HM_TRACE( RequestSucceeded, 25, Noise, "{0}+{1}: process message {2}: success", "prId", "activityId", "messageId" )
            HM_TRACE( RequestFailed, 26, Info, "{0}+{1}: process message {2}: error = {3}", "prId", "activityId", "messageId", "error" )
            HM_TRACE( InvalidAction, 27, Info, "{0}+{1}: ({2}) message {3}: action not supported", "prId", "activityId", "action", "messageId" )
            HM_TRACE( InternalMethodFailed, 28, Noise, "{1}: {2} failed: {3} {4}", "entityId", "activityId", "methodName", "error", "errorMessage")

            // Report processing
            //
            HM_TRACE( DropAttribute, 30, Noise, "{0}: Attribute {1} ({2}) doesn't match expected name", "raId", "attributeName", "attributeValue" )
            HM_TRACE( DropReportStaleSourceLSN, 32, Info, "{0}: {1}: Drop report: current source LSN={2}, incoming={3}", "raId", "pk", "crtSourceLSN", "newSourceLSN" )
            HM_TRACE( DropReportInvalidSourceLSN, 33, Info, "{0}: Drop {1}", "raId", "report" )
            HM_TRACE( ProcessReportFailed, 35, Info, "{1} {2} failed: {3} {4}, original timeout {5}", "id", "context", "attributes", "error", "errorMessage", "timeout" )
            HM_TRACE( PersistEventPreviousExpired, 36, Info, "{1}: previous event (sn {2}) is expired, needs persist", "id", "activityId", "sn" )
            HM_TRACE( ProcessReportUpdateAttributes, 37, Info, "{1}: ProcessReport updated attributes: {2}, inMemoryOnly={3}, {4}", "id", "activityId", "attr", "inMemoryOnly", "entityState" )
            HM_TRACE( LoadEntityDataFailed, 38, Warning, "{1}+{2}: Events store read failed: {3}", "id", "prId", "activityId", "error" )
            Complete_ProcessReport(Common::TraceTaskCodes::HealthManager, 34, "Complete_ProcessReport", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, CustomerInfo), "Report {1} {2} {3} completed successfully. {4}", "id", "activityId", "report", "attributes", "details"),

            // Entity & entity cache traces
            //
            HM_TRACE( BlockReport, 39, Noise, "Block {1}, cache not initialized", "id", "context" )
            HM_TRACE( CleanupTimer, 40, Info, "{0}+{1}: {2} cache: cleanup callback, entity count: {3}/{4}, job items scheduled={5},skipped={6},alreadyScheduled={7}", "prId", "activityId", "cacheType", "count", "newCount", "scheduledJI", "skippedJI", "previouslyScheduledJI")
            HM_TRACE( Cleanup, 41, Info, "{1}: start cleanup {2}", "id", "raId", "entity" )
            HM_TRACE( CleanupExpiredTransientEventsStart, 42, Info, "{0}: start cleanup {1}: {2} expired transient events, {3} mismatched expired flag", "raId", "entity", "count", "expiredMismatch" )
            HM_TRACE( ReadFailed, 43, Info, "{0}+{1}: {2} cache: initial store load failed: {3}", "prId", "activityId", "entityType", "error" )
            HM_TRACE( ReadComplete, 44, Info, "{0}+{1}: {2} cache: initial store load successful with {3} entities, {4} events", "prId", "activityId", "entityType", "entityCount", "eventCount" )
            HM_TRACE( GetHealth, 45, Noise, "{1}: {2}: {3}: Events (ok:{4}/w:{5}/e:{6}, expired:{7}, considerWarningAsError:{8}): {9}, hasAuthoritySource {10}", "id", "prId", "aId", "entity", "okCount", "warningCount", "errorCount", "expiredCount", "policy", "healthState", "hasAuthoritySource" )
            HM_TRACE( NoParent, 46, Info, "{1}: {0} No parent {2} found", "id", "activityId", "parentType" )
            HM_TRACE( SetParent, 47, Info, "{1}: {0}: Set parent to {2}.", "id", "activityId", "parentEntity" )
            HM_TRACE( DeleteChild, 48, Info, "{1}: {2}: Mark for delete due to parent {3}", "id", "prId", "entity", "parentEntity" )
            HM_TRACE( EntityState, 49, Noise, "{1}: {2}: parentsHealthy={3}, deleteDueToParent={4}, hasHierarchyReport={5}", "id", "prId", "entity", "parentsHealthy", "deleteDueToParent", "hasHierarchyReport" )

            // Query traces
            //
            HM_TRACE( QueryNoArg, 50, Info, "{0}: Missing required argument {1}", "raId", "argType" )
            HM_TRACE( QueryParseArg, 51, Info, "{0}: Error parsing argument {1}={2}", "raId", "argType", "arg" )
            HM_TRACE( QueryInvalidConfig, 52, Info, "{0}: Error parsing query arguments for {1}: {2}", "raId", "queryName", "error" )
            HM_TRACE( Entity_QueryCompleted, 53, Info, "{1} completed: {2} Filters: [{3}].", "id", "context", "health", "filters" )
            HM_TRACE( QueryFailed, 54, Info, "{0} failed with {1} {2}", "context", "error", "message" )
            HM_TRACE( QueryReceived, 55, Noise, "Received {0}", "context" )
            HM_TRACE( QueryListSkip, 57, Noise, "{0}: skip adding {1} due to {2}", "context", "entity", "error" )
            HM_TRACE( List_QueryCompleted, 58, Info, "{0} completed with {1} results, ContinuationToken='{2}'", "context", "resultCount", "continuationToken" )
            HM_TRACE( Cluster_QueryCompleted, 61, Info, "{1} completed: {2} Filters: [{3}]\r\n Children evaluation time: {4}.", "id", "context", "health", "filters", "elapsed" )
            HM_TRACE( Cluster_ChunkQueryCompleted, 64, Info, "{1} completed: {2}, {3} applications, {4} nodes. Filters: [{5}]", "id", "context", "healthState", "appCount", "nodeCount", "filters")
            HM_TRACE( Query_MaxMessageSizeReached, 65, Info, "{1}: reached max message size with {2}: {3} {4}", "id", "activityId", "entityId", "error", "errorMessage")
            HM_TRACE( GetEventsHealthStateFailedDueToState, 66, Info, "{1}: GetEventsHealthState failed due to state {2}", "id", "activityId", "entityState" )
            HM_TRACE( DropQuery, 67, Info, "{0}: Drop query {1}: {2} {3}", "traceId", "queryName", "error", "errorMessage" )
            HM_TRACE( Cluster_QueryCompletedCached, 68, Info, "{1} completed with cached value: {2} Filters: [{3}]", "id", "context", "health", "filters" )
            HM_TRACE( Query_MaxResultsReached, 69, Info, "{1}: reached max results limit with {2}: {3} {4}", "id", "activityId", "entityId", "error", "errorMessage" )

            // Sequence stream processing traces
            //
            HM_TRACE( CachePersistSS, 71, Info, "{0}: Persist {1}", "raId", "ss" )
            HM_TRACE( CacheQueuedReports, 72, Noise, "{0}: Queued {1} reports for processing, pending count {2}/critical {3}, pending size {4}/critical {5}", "raId", "count", "pending", "pendingCritical", "pendingSize", "pendingCriticalSize" )
            HM_TRACE( ReportReplyAddSS, 73, Info, "Ack {0}. {1} pending requests", "context", "pending" )
            HM_TRACE( ReportReplySSFailed, 74, Info, "Processing {0} failed with {1}. {2} pending requests", "context", "error", "pending" )
            HM_TRACE( GetSSProgressFailed, 76, Info, "{0}: Cache {1}: Get SS failed with {2}", "prId", "kind", "error" )
            HM_TRACE( BlockSS, 77, Noise, "Block {0}, cache not initialized", "context" )
            HM_TRACE( QueueReports, 78, Info, "{0}: Queued {1} reports, rejected {2}, pending count {3}/critical {4}, pending size {5}/critical {6}", "raId", "acceptedCount", "rejectedCount", "pending", "pendingCritical", "pendingSize", "pendingCriticalSize")

            // Replicated store traces
            //
            HM_TRACE( CommitFailed, 84, Info, "{0}: Commit tx failed: {1} {2}", "raId", "error", "errorMessage" )

            // Specific entity-derived traces
            //
            HM_TRACE( UpdateParent, 103, Noise, "{1}: {0}: Update parent attrib to {2}", "id", "activityId", "parentEntity" )
            HM_TRACE( CreateNonPersisted, 107, Info, "{1}: {2}: Create non-persisted entity {0}", "id", "raId", "kind" )
            HM_TRACE( ChildrenState, 108, Noise, "{1}: {2}: {3} {4}, state={5} maxUnhealthy={6}", "id", "pr", "aId", "childType", "healthCount", "healthState", "maxPercentUnhealthy" )
            HM_TRACE( ServiceType_ChildrenState, 109, Noise, "{1}: {2}: {3}: {4} {5} state={6} maxUnhealthy={7}", "id", "pr", "aId", "serviceType", "childType", "healthCount", "healthState", "maxPercentUnhealthy" )
            HM_TRACE( NoSystemReport, 110, Noise, "{1}: {2}: no system report", "id", "prId", "entity" )
            HM_TRACE( AddChild, 111, Noise, "{1}: Add child {2}", "id", "prId", "child" )
            HM_TRACE( NoHealthPolicy, 112, Info, "{1}: {2}: policy not set", "id", "raId", "entity" )
            HM_TRACE( AppNotInCache, 113, Noise, "{1}: {2}: app not in cache", "id", "raId", "entity" )
            HM_TRACE( UnhealthyChildrenPerUd, 114, Warning, "{1}: {2}: {3} in ud {4} unhealthy: {5} state={6} maxUnhealthy={7}", "id", "pr", "aId", "childType", "ud", "healthCount", "healthState", "maxPercentUnhealthy" )
            HM_TRACE( ChildrenPerUd, 115, Info, "{1}: {2}: {3} in ud {4} healthy: {5} state={6} maxUnhealthy={7}", "id", "pr", "aId", "childType", "ud", "healthCount", "healthState", "maxPercentUnhealthy" )
            HM_TRACE( HealthReason, 116, Info, "{1}: {2}: state={3}: \r\r {4}", "id", "pr", "aId", "healthState", "reason" )
            HM_TRACE( PersistReportExpiredMismatch, 117, Info, "{1}: Need to persist {2}+{3}, sn {4} with expired {5}", "id", "rId", "source", "property", "sn", "expired")
            HM_TRACE( CheckInMemoryAgainstStoreData, 118, Info, "{1}: consistent check completed: {2}, duration: {3}. {4}", "id", "activityId", "error", "duration", "attributes" )
            HM_TRACE( ReserveCleanupJobFailed, 119, Info, "{1}: TryReservePendingCleanupJob: entity already has cleanup jobs, reserve failed", "id", "activityId" )
            HM_TRACE( DeleteEntityFromStore, 120, Info, "{1}: Delete entity from store: {2}, state {3}", "id", "activityId", "attr", "entityState" )
            HM_TRACE( ProcessReportTimedOut, 121, Info, "{0}: timed out, {1} still pending reports. Send partial results (S_OK)", "traceId", "pendingCount" )
            HM_TRACE( CreateCleanupJobFailed, 122, Info, "{1}: Can't create cleanup job item: pending count {2}, max {3}", "id", "activityId", "count", "maxCount" )
            HM_TRACE( CacheSlowStats, 123, Info, "{1}: Cache entity stats, elapsed {2}, stats {3}", "id", "activityId", "elapsed", "stats" )
            HM_TRACE( DisableCacheStatsQuick, 124, Info, "{1}: Disable cached entity stats, elapsed {2}", "id", "activityId", "elapsed" )
            HM_TRACE( DisableCacheStatsOnError, 125, Info, "{1}: Disable cached entity stats, compute health failed: {2}", "id", "activityId", "error" )
            HM_TRACE( CreateNonPersistedForLeakedEvents, 126, Warning, "{0}: {1}: Create non-persisted entity. Found {2} leaked events. First event: {3}", "activityId", "kind", "eventCount", "firstEvent" )
            HM_TRACE( CleanupLeakedEvents, 127, Info, "{1}: start cleanup {2} events for {3}", "id", "raId", "eventCount", "entity" )

            // Upgrade related traces
            //
            HM_TRACE( AppHealthy, 130, Info, "{1}: app {0} upgrade domains {2} health check complete: {3}, healthy.", "id", "activityId", "upgradeDomains", "healthState" )
            HM_TRACE( ClusterUpgradeSkipNode, 131, Info, "{0}: Skip node {1}, NodeName not set: {2}", "activityId", "entityId", "attr")
            HM_TRACE( UpgradeGlobalDeltaRespected, 132, Info, "{0}: Global delta is respected: baseline error:{1}/total:{2}, current {3}, delta {4}", "activityId", "baselineErrorCount", "baselineTotalCount", "healthCount", "percentDeltaUnhealthy" )
            HM_TRACE( UpgradeGlobalDeltaNotRespected, 133, Warning, "{0}: {1}: Global delta is not respected: baseline error:{2}/total:{3}, current {4}, delta {5}", "rId", "activityId", "baselineErrorCount", "baselineTotalCount", "healthCount", "percentDeltaUnhealthy" )
            HM_TRACE( UpgradeUDDeltaRespected, 134, Info, "{0}: UD {1}: UD delta is respected: baseline error:{2}/total:{3}, current {4}, delta {5}", "activityId", "upgradeDomainName", "baselineErrorCount", "baselineTotalCount", "healthCount", "percentDeltaUnhealthy" )
            HM_TRACE( UpgradeUDDeltaNotRespected, 135, Warning, "{0}: {1}: UD delta is not respected: UD {2}: baseline error:{3}/total:{4}, current {5}, delta {6}", "rId", "activityId", "upgradeDomainName", "baselineErrorCount", "baselineTotalCount", "healthCount", "percentDeltaUnhealthy" )
            HM_TRACE( QuerySkipNode, 136, Info, "{0}: Skip node {1}, NodeName not set: {2}", "activityId", "entityId", "attr")
            HM_TRACE( SystemAppUnhealthy, 137, Info, "{0}: system app health is {1}", "activityId", "healthState" )
            HM_TRACE( ClusterHealthy, 138, Info, "{0}:{1} cluster health check complete: {2}, healthy.", "rId", "activityId", "healthState" )
            HM_TRACE( ClusterHealthyWithEvaluations, 139, Info, "{0}:{1} cluster health check complete: {2}, healthy. {3}", "rId", "activityId", "healthState", "unhealthyEvals" )
            HM_TRACE( UpgradeClusterUnhealthy, 140, Warning, "{0}:{1} cluster is not healthy for upgrade: {2}: {3}", "rId", "activityId", "healthState", "unhealthyEvals" )
            HM_TRACE( UpgradeAppUnhealthy, 141, Warning, "{1}: app {0} is in Error during upgrade: {2}", "id", "activityId", "unhealthyEvals" )

            // Process Report For Event Store
            //

            PARTITIONS_OPERATIONAL_TRACE(
                ProcessPartitionReportOperational,
                L"PartitionNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                150,
                Info,
                "Partition={1} SourceId={2} Property={3} HealthState={4} TTL={5} SequenceNumber={6} Description='{7}' RemoveWhenExpired={8} SourceUTCTimestamp={9}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            NODES_OPERATIONAL_TRACE(
                ProcessNodeReportOperational,
                L"NodeNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                151,
                Info,
                "Node={1} NodeInstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "nodeInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            SERVICES_OPERATIONAL_TRACE(
                ProcessServiceReportOperational,
                L"ServiceNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                152,
                Info,
                "Service={1} InstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "instanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ProcessApplicationReportOperational,
                L"ApplicationNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                153,
                Info,
                "Application={1} InstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "applicationInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ProcessDeployedApplicationReportOperational,
                L"DeployedApplicationNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                154,
                Info,
                "DeployedApplication={1} InstanceId={2} NodeName={3} SourceId={4} Property={5} HealthState={6} TTL={7} SequenceNumber={8} Description='{9}' RemoveWhenExpired={10} SourceUTCTimestamp={11}",
                "applicationInstanceId",
                "nodeName",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ProcessDeployedServicePackageReportOperational,
                L"DeployedServicePackageNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                155,
                Info,
                "ApplicationName={1} ServiceManifest={2} InstanceId={3} ServicePackageActivationId={4} NodeName={5} SourceId={6} Property={7} HealthState={8} TTL={9} SequenceNumber={10} Description='{11}' RemoveWhenExpired={12} SourceUTCTimestamp={13}",
                "serviceManifestName",
                "servicePackageInstanceId",
                "servicePackageActivationId",
                "nodeName",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            CLUSTER_OPERATIONAL_TRACE(
                ProcessClusterReportOperational,
                L"ClusterNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                156,
                Info,
                "SourceId={1} Property={2} HealthState={3} TTL={4} SequenceNumber={5} Description='{6}' RemoveWhenExpired={7} SourceUTCTimestamp={8}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            REPLICAS_OPERATIONAL_TRACE(
                ProcessStatefulReplicaReportOperational,
                L"StatefulReplicaNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                157,
                Info,
                "Partition={1} StatefulReplica={2} InstanceId={3} SourceId={4} Property={5} HealthState={6} TTL={7} SequenceNumber={8} Description='{9}' RemoveWhenExpired={10} SourceUTCTimestamp={11}",
                "replicaInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            REPLICAS_OPERATIONAL_TRACE(
                ProcessStatelessInstanceReportOperational,
                L"StatelessReplicaNewHealthReport",
                OperationalHealthCategory,
                HealthManager,
                158,
                Info,
                "Partition={1} StatelessInstance={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            // Message string cannot be exact same as others, so adding "Expired: " to differentiate.
            PARTITIONS_OPERATIONAL_TRACE(
                ExpiredPartitionEventOperational,
                L"PartitionHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                159,
                Info,
                "Expired: Partition={1} SourceId={2} Property={3} HealthState={4} TTL={5} SequenceNumber={6} Description='{7}' RemoveWhenExpired={8} SourceUTCTimestamp={9}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            NODES_OPERATIONAL_TRACE(
                ExpiredNodeEventOperational,
                L"NodeHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                160,
                Info,
                "Expired: Node={1} NodeInstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "nodeInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            SERVICES_OPERATIONAL_TRACE(
                ExpiredServiceEventOperational,
                L"ServiceHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                161,
                Info,
                "Expired: Service={1} InstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "instanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredApplicationEventOperational,
                L"ApplicationHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                162,
                Info,
                "Expired: Application={1} InstanceId={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "applicationInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredDeployedApplicationEventOperational,
                L"DeployedApplicationHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                163,
                Info,
                "Expired: DeployedApplication={1} InstanceId={2} NodeName={3} SourceId={4} Property={5} HealthState={6} TTL={7} SequenceNumber={8} Description='{9}' RemoveWhenExpired={10} SourceUTCTimestamp={11}",
                "applicationInstanceId",
                "nodeName",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredDeployedServicePackageEventOperational,
                L"DeployedServicePackageHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                164,
                Info,
                "Expired: ApplicationName={1} ServiceManifest={2} InstanceId={3} ServicePackageActivationId={4} NodeName={5} SourceId={6} Property={7} HealthState={8} TTL={9} SequenceNumber={10} Description='{11}' RemoveWhenExpired={12} SourceUTCTimestamp={13}",
                "serviceManifest",
                "servicePackageInstanceId",
                "servicePackageActivationId",
                "nodeName",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            CLUSTER_OPERATIONAL_TRACE(
                ExpiredClusterEventOperational,
                L"ClusterHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                165,
                Info,
                "Expired: SourceId={1} Property={2} HealthState={3} TTL={4} SequenceNumber={5} Description='{6}' RemoveWhenExpired={7} SourceUTCTimestamp={8}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            REPLICAS_OPERATIONAL_TRACE(
                ExpiredStatefulReplicaEventOperational,
                L"StatefulReplicaHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                166,
                Info,
                "Expired: Partition={1} StatefulReplica={2} InstanceId={3} SourceId={4} Property={5} HealthState={6} TTL={7} SequenceNumber={8} Description='{9}' RemoveWhenExpired={10} SourceUTCTimestamp={11}",
                "replicaInstanceId",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

            REPLICAS_OPERATIONAL_TRACE(
                ExpiredStatelessInstanceEventOperational,
                L"StatelessReplicaHealthReportExpired",
                OperationalHealthCategory,
                HealthManager,
                167,
                Info,
                "Expired: Partition={1} StatelessInstance={2} SourceId={3} Property={4} HealthState={5} TTL={6} SequenceNumber={7} Description='{8}' RemoveWhenExpired={9} SourceUTCTimestamp={10}",
                "sourceId",
                "property",
                "healthState",
                "TTL",
                "sequenceNumber",
                "description",
                "removeWhenExpired",
                "sourceUtcTimestamp"),

        END_HM_STRUCTURED_TRACES

            // Declarations

            // Replica Life-cycle
            //
            DECLARE_HM_TRACE( ReplicaCtor, std::wstring, Federation::NodeInstance, __int64 )
            DECLARE_HM_TRACE( ReplicaDtor, std::wstring, __int64 )
            DECLARE_HM_TRACE( ReplicaOpen, std::wstring, Federation::NodeInstance )
            DECLARE_HM_TRACE( ReplicaChangeRole, std::wstring, std::wstring )
            DECLARE_HM_TRACE( ReplicaClose, std::wstring )
            DECLARE_HM_TRACE( ReplicaAbort, std::wstring )
            DECLARE_HM_TRACE( ReplicaCloseCompleted, std::wstring, Common::ErrorCode )

            // General Request Processing
            //

            DECLARE_HM_TRACE( InvalidMessage, std::wstring, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_HM_TRACE( InvalidMessage2, std::wstring, Common::ActivityId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_HM_TRACE( InvalidActor, std::wstring, Common::ActivityId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_HM_TRACE( RequestReceived, std::wstring, Common::ActivityId, std::wstring, Transport::MessageId )
            DECLARE_HM_TRACE( RequestRejected, std::wstring, Common::ActivityId, Transport::MessageId, Common::ErrorCode )
            DECLARE_HM_TRACE( RequestSucceeded, std::wstring, Common::ActivityId, Transport::MessageId )
            DECLARE_HM_TRACE( RequestFailed, std::wstring, Common::ActivityId, Transport::MessageId, Common::ErrorCode )
            DECLARE_HM_TRACE( InvalidAction, std::wstring, Common::ActivityId, std::wstring, Transport::MessageId )
            DECLARE_HM_TRACE( InternalMethodFailed, std::wstring, Common::ActivityId, std::wstring, Common::ErrorCode, std::wstring )

            // Report processing
            //
            DECLARE_HM_TRACE( DropAttribute, Store::ReplicaActivityId, std::wstring, std::wstring )
            DECLARE_HM_TRACE( DropReportStaleSourceLSN, Store::ReplicaActivityId, std::wstring, int64, int64 )
            DECLARE_HM_TRACE( DropReportInvalidSourceLSN, std::wstring, ServiceModel::HealthReport)
            DECLARE_HM_TRACE( ProcessReportFailed, std::wstring, ReportRequestContext, std::wstring, Common::ErrorCode, std::wstring, Common::TimeSpan )
            DECLARE_HM_TRACE( LoadEntityDataFailed, std::wstring, std::wstring, Common::ActivityId, Common::ErrorCode )
            DECLARE_HM_TRACE( PersistEventPreviousExpired, std::wstring, Common::ActivityId, FABRIC_SEQUENCE_NUMBER )
            DECLARE_HM_TRACE( ProcessReportUpdateAttributes, std::wstring, Common::ActivityId, AttributesStoreData, bool, HealthEntityState::Trace )

            // Entity and entity cache traces
            //
            DECLARE_HM_TRACE( BlockReport, std::wstring, ReportRequestContext )
            DECLARE_HM_TRACE( CleanupTimer, std::wstring, Common::ActivityId, std::wstring, int32, int32, int, int, int )
            DECLARE_HM_TRACE( Cleanup, std::wstring, std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( CleanupExpiredTransientEventsStart, std::wstring, AttributesStoreData, uint64, uint64 )
            DECLARE_HM_TRACE( ReadFailed, std::wstring, Common::ActivityId, std::wstring, Common::ErrorCode )
            DECLARE_HM_TRACE( ReadComplete, std::wstring, Common::ActivityId, std::wstring, uint64, uint64 )
            DECLARE_HM_TRACE( GetHealth, std::wstring, std::wstring, Common::ActivityId, AttributesStoreData, int, int, int, int, bool, std::wstring, bool )
            DECLARE_HM_TRACE( NoParent, std::wstring, Common::ActivityId, HealthEntityKind::Trace )
            DECLARE_HM_TRACE( SetParent, std::wstring, Common::ActivityId, AttributesStoreData )
            DECLARE_HM_TRACE( DeleteChild, std::wstring, std::wstring, AttributesStoreData, AttributesStoreData )
            DECLARE_HM_TRACE( EntityState, std::wstring, std::wstring, AttributesStoreData, bool, bool, bool )

            // Query traces
            //
            DECLARE_HM_TRACE( QueryNoArg, std::wstring, std::wstring )
            DECLARE_HM_TRACE( QueryParseArg, std::wstring, std::wstring, std::wstring )
            DECLARE_HM_TRACE( QueryInvalidConfig, std::wstring, std::wstring, Common::ErrorCode )
            DECLARE_HM_TRACE( Entity_QueryCompleted, std::wstring, QueryRequestContext, ServiceModel::EntityHealthBase, std::wstring)
            DECLARE_HM_TRACE( QueryFailed, QueryRequestContext, Common::ErrorCode, std::wstring )
            DECLARE_HM_TRACE( QueryReceived, QueryRequestContext )
            DECLARE_HM_TRACE( QueryListSkip, QueryRequestContext, std::wstring, Common::ErrorCodeValue::Trace )
            DECLARE_HM_TRACE( List_QueryCompleted, QueryRequestContext, uint64, std::wstring )
            DECLARE_HM_TRACE( Cluster_QueryCompleted, std::wstring, QueryRequestContext, ServiceModel::ClusterHealth, std::wstring, Common::TimeSpan )
            DECLARE_HM_TRACE( Cluster_ChunkQueryCompleted, std::wstring, QueryRequestContext, std::wstring, uint64, uint64, std::wstring )
            DECLARE_HM_TRACE( Query_MaxMessageSizeReached, std::wstring, Common::ActivityId, std::wstring, Common::ErrorCode, std::wstring )
            DECLARE_HM_TRACE( GetEventsHealthStateFailedDueToState, std::wstring, Common::ActivityId, HealthEntityState::Trace )
            DECLARE_HM_TRACE( DropQuery, std::wstring, Query::QueryNames::Trace, Common::ErrorCode, std::wstring )
            DECLARE_HM_TRACE( Cluster_QueryCompletedCached, std::wstring, QueryRequestContext, ServiceModel::ClusterHealth, std::wstring )
            DECLARE_HM_TRACE( Query_MaxResultsReached, std::wstring, Common::ActivityId, std::wstring, Common::ErrorCode, std::wstring )

            // Sequence stream processing traces
            //
            DECLARE_HM_TRACE( CachePersistSS, std::wstring, SequenceStreamStoreData )
            DECLARE_HM_TRACE( CacheQueuedReports, std::wstring, uint64, int64, int64, int64, int64 )
            DECLARE_HM_TRACE( ReportReplyAddSS, SequenceStreamRequestContext, uint64 )
            DECLARE_HM_TRACE( ReportReplySSFailed, SequenceStreamRequestContext, Common::ErrorCodeValue::Trace, uint64 )
            DECLARE_HM_TRACE( GetSSProgressFailed, std::wstring, std::wstring, Common::ErrorCodeValue::Trace);
            DECLARE_HM_TRACE( BlockSS, SequenceStreamRequestContext )
            DECLARE_HM_TRACE( QueueReports, std::wstring, uint64, uint64, int64, int64, int64, int64 )

            // Replicated store traces
            //
            DECLARE_HM_TRACE( CommitFailed, std::wstring, Common::ErrorCode, std::wstring )

            // More entity traces
            //
            DECLARE_HM_TRACE( UpdateParent, std::wstring, Common::ActivityId, AttributesStoreData )
            DECLARE_HM_TRACE( CreateNonPersisted, std::wstring, std::wstring, std::wstring)
            DECLARE_HM_TRACE( ChildrenState, std::wstring, std::wstring, Common::ActivityId, HealthEntityKind::Trace, HealthCount, std::wstring, UCHAR )
            DECLARE_HM_TRACE( ServiceType_ChildrenState, std::wstring, std::wstring, Common::ActivityId, std::wstring, HealthEntityKind::Trace, HealthCount, std::wstring, UCHAR )
            DECLARE_HM_TRACE( NoSystemReport, std::wstring, std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( AddChild, std::wstring, std::wstring, std::wstring )
            DECLARE_HM_TRACE( NoHealthPolicy, std::wstring, std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( AppNotInCache, std::wstring,std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( UnhealthyChildrenPerUd, std::wstring, std::wstring, Common::ActivityId, HealthEntityKind::Trace, std::wstring, HealthCount, std::wstring, UCHAR )
            DECLARE_HM_TRACE( ChildrenPerUd, std::wstring, std::wstring, Common::ActivityId, HealthEntityKind::Trace, std::wstring, HealthCount, std::wstring, UCHAR )
            DECLARE_HM_TRACE( HealthReason, std::wstring, std::wstring, Common::ActivityId, std::wstring, ServiceModel::HealthEvaluationBase)
            DECLARE_HM_TRACE( PersistReportExpiredMismatch, std::wstring, std::wstring, std::wstring, std::wstring, int64, bool )
            DECLARE_HM_TRACE( CheckInMemoryAgainstStoreData, std::wstring, Common::ActivityId, Common::ErrorCode, Common::TimeSpan, AttributesStoreData )
            DECLARE_HM_TRACE( ReserveCleanupJobFailed, std::wstring, Common::ActivityId )
            DECLARE_HM_TRACE( DeleteEntityFromStore, std::wstring, Common::ActivityId, AttributesStoreData, HealthEntityState::Trace )
            DECLARE_HM_TRACE( ProcessReportTimedOut, std::wstring, uint64 )
            DECLARE_HM_TRACE( CreateCleanupJobFailed, std::wstring, Common::ActivityId, uint64, int )
            DECLARE_HM_TRACE( CacheSlowStats, std::wstring, Common::ActivityId, Common::TimeSpan, std::wstring )
            DECLARE_HM_TRACE( DisableCacheStatsQuick, std::wstring, Common::ActivityId, Common::TimeSpan )
            DECLARE_HM_TRACE( DisableCacheStatsOnError, std::wstring, Common::ActivityId, Common::ErrorCode )
            DECLARE_HM_TRACE( CreateNonPersistedForLeakedEvents, Common::ActivityId, std::wstring, uint64, std::wstring )
            DECLARE_HM_TRACE( CleanupLeakedEvents, std::wstring, std::wstring, uint64, AttributesStoreData )

            // Upgrade related traces
            //
            DECLARE_HM_TRACE( AppHealthy, std::wstring, Common::ActivityId, std::wstring, std::wstring )
            DECLARE_HM_TRACE( ClusterUpgradeSkipNode, Common::ActivityId, std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( UpgradeGlobalDeltaRespected, Common::ActivityId, uint64, uint64, HealthCount, UCHAR )
            DECLARE_HM_TRACE( UpgradeGlobalDeltaNotRespected, std::wstring, Common::ActivityId, uint64, uint64, HealthCount, UCHAR )
            DECLARE_HM_TRACE( UpgradeUDDeltaRespected, Common::ActivityId, std::wstring, uint64, uint64, HealthCount, UCHAR )
            DECLARE_HM_TRACE( UpgradeUDDeltaNotRespected, std::wstring, Common::ActivityId, std::wstring, uint64, uint64, HealthCount, UCHAR )
            DECLARE_HM_TRACE( QuerySkipNode, Common::ActivityId, std::wstring, AttributesStoreData )
            DECLARE_HM_TRACE( SystemAppUnhealthy, Common::ActivityId, std::wstring )
            DECLARE_HM_TRACE( ClusterHealthy, std::wstring, Common::ActivityId, std::wstring )
            DECLARE_HM_TRACE( ClusterHealthyWithEvaluations, std::wstring, Common::ActivityId, std::wstring, ServiceModel::HealthEvaluationBase )
            DECLARE_HM_TRACE( UpgradeClusterUnhealthy, std::wstring, Common::ActivityId, std::wstring, ServiceModel::HealthEvaluationBase )
            DECLARE_HM_TRACE( UpgradeAppUnhealthy, std::wstring, Common::ActivityId, ServiceModel::HealthEvaluationBase )

            // Trace process health report for reports which may affect health evaluation

            DECLARE_PARTITIONS_OPERATIONAL_TRACE(
                ProcessPartitionReportOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_NODES_OPERATIONAL_TRACE(
                ProcessNodeReportOperational,
                FABRIC_NODE_INSTANCE_ID, // NodeInstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_SERVICES_OPERATIONAL_TRACE(
                ProcessServiceReportOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ProcessApplicationReportOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ProcessDeployedApplicationReportOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // NodeName
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ProcessDeployedServicePackageReportOperational,
                std::wstring, // ServiceManifestName
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // ServicePackageActivationId
                std::wstring, // NodeName
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_CLUSTER_OPERATIONAL_TRACE(
                ProcessClusterReportOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_REPLICAS_OPERATIONAL_TRACE(
                ProcessStatefulReplicaReportOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_REPLICAS_OPERATIONAL_TRACE(
                ProcessStatelessInstanceReportOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            // Trace expired events which potentially affect health evaluation

            DECLARE_PARTITIONS_OPERATIONAL_TRACE(
                ExpiredPartitionEventOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_NODES_OPERATIONAL_TRACE(
                ExpiredNodeEventOperational,
                FABRIC_NODE_INSTANCE_ID, // NodeInstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_SERVICES_OPERATIONAL_TRACE(
                ExpiredServiceEventOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredApplicationEventOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredDeployedApplicationEventOperational,
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // NodeName
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_APPLICATIONS_OPERATIONAL_TRACE(
                ExpiredDeployedServicePackageEventOperational,
                std::wstring, // ServiceManifestName
                FABRIC_INSTANCE_ID, // InstanceId
                std::wstring, // ServicePackageActivationId
                std::wstring, // NodeName
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_CLUSTER_OPERATIONAL_TRACE(
                ExpiredClusterEventOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_REPLICAS_OPERATIONAL_TRACE(
                ExpiredStatefulReplicaEventOperational,
                FABRIC_INSTANCE_ID, // ReplicaInstanceId
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            DECLARE_REPLICAS_OPERATIONAL_TRACE(
                ExpiredStatelessInstanceEventOperational,
                std::wstring, // SourceId - user input
                std::wstring, // Property - user input
                ServiceModel::HealthState::Trace, // HealthState
                Common::TimeSpan, // TTL
                FABRIC_SEQUENCE_NUMBER, // Sequence Number
                std::wstring, // Description
                bool, // RemoveWhenExpired
                Common::DateTime) // SourceUtcTime

            Common::TraceEventWriter<std::wstring, Common::ActivityId, ServiceModel::HealthReport, std::wstring, std::wstring> Complete_ProcessReport;

            // Trace Process Report for each entity
            void TraceProcessReportOperational(
                ServiceModel::HealthReport const & healthReport);

            void TraceEventExpiredOperational(
                HealthEntityKind::Enum entityKind,
                HealthEventStoreDataUPtr const & eventEntry,
                AttributesStoreDataSPtr const & attributes);

            static Common::Global<HealthManagerEventSource> Trace;

            // Determines if you should trace to the operational channel, based on current and previous health states.
            static bool ShouldTraceToOperationalChannel(FABRIC_HEALTH_STATE previousState, FABRIC_HEALTH_STATE currentState);
        };

        typedef HealthManagerEventSource HMEvents;
    }
}
