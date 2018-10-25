// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class EventSource
        {
        public:
            Common::TraceEventWriter<std::wstring, Federation::NodeId, uint64> Activated;
            Common::TraceEventWriter<std::wstring, Federation::NodeId, uint64> Deactivated;
            Common::TraceEventWriter<std::wstring, Federation::NodeId, uint64, Common::TimeSpan> Ready;
            Common::TraceEventWriter<std::string, int32, int32, int32> Nodes;
            Common::TraceEventWriter<Common::Guid, std::wstring, int64, int64, std::wstring, std::wstring, std::wstring, std::wstring> Movement;
            Common::TraceEventWriter<Common::Guid, std::wstring, int64, int64, std::wstring, std::wstring, std::wstring> SwapPrimary;
            Common::TraceEventWriter<std::wstring, std::wstring, int64, int64, std::wstring, Federation::NodeId, std::wstring> Drop;
            Common::TraceEventWriter<uint16, std::wstring, Federation::NodeInstance, int64, int64, uint64, ServiceModel::ServicePackageVersionInstance, Common::DateTime, Common::DateTime> FMReplica;
            Common::TraceEventWriter<uint16, std::wstring, int, int, FailoverUnitDescription, std::wstring, int64, int64, int64, int64, bool, Common::DateTime, std::vector<Replica>> FMFailoverUnit;
            Common::TraceEventWriter<uint16, std::wstring> StateMachineAction;
            Common::TraceEventWriter<QueueCounts> QueueCounts;
            
            Common::TraceEventWriter<uint64> ClusterInPauseState;
            Common::TraceEventWriter<uint64> ClusterEndPauseState;
            Common::TraceEventWriter<Common::Guid, Federation::NodeInstance, int64, int64, Common::StringLiteral, std::wstring, std::wstring> EndpointsUpdated;

            Common::TraceEventWriter<std::wstring, PeriodicTaskName::Trace> PeriodicTaskBegin;
            Common::TraceEventWriter<std::wstring, PeriodicTaskName::Trace> PeriodicTaskBeginNoise;
            Common::TraceEventWriter<std::wstring, bool, uint64, int, int64> FTPeriodicTaskEnd;
            Common::TraceEventWriter<bool, bool, int> BackgroundEnumerationAborted;
            Common::TraceEventWriter<> BackgroundThreadStart;
            Common::TraceEventWriter<int, size_t, int, int, bool, bool> BackgroundThreadEndStatistics;
            Common::TraceEventWriter<int, size_t, int, bool> BackgroundThreadIterationEnd;
            Common::TraceEventWriter<std::wstring, bool> BackgroundThreadContextCompleted;

            Common::TraceEventWriter<FailoverUnitCountsContext::TraceWriter, bool> FailoverUnitCounts;
            Common::TraceEventWriter<Common::Guid, std::wstring, int64, int64, std::wstring, Federation::NodeId> PromoteSecondary;
            Common::TraceEventWriter<Common::Guid, std::wstring, int64, int64, std::wstring, Federation::NodeId> VoidMovement;
            Common::TraceEventWriter<PlbApiCallName::Trace, int64> PlbFunctionCallSlow;
            Common::TraceEventWriter<ServiceModel::ApplicationIdentifier, std::wstring, int64> CreateApplicationSuccess;
            Common::TraceEventWriter<std::wstring, ServiceModel::ApplicationIdentifier, int64, int64> AppRemoved;

            Common::TraceEventWriter<std::wstring, std::wstring, bool, size_t, size_t, size_t, size_t, size_t, size_t, bool> AppUpgradeInProgress;
            Common::TraceEventWriter<std::wstring, bool, size_t, size_t, size_t, size_t, size_t, size_t, bool> FabricUpgradeInProgress;

            Common::TraceEventWriter<QueueName::Trace, size_t> OnQueueFull;

            Common::TraceEventWriter<Common::Guid, std::wstring> OnQueueTimeout;

            Common::TraceEventWriter<ServiceModel::ApplicationIdentifier> PLBSafetyCheckForApplicationReceived;
            Common::TraceEventWriter<ServiceModel::ApplicationIdentifier, Common::ErrorCode>PLBSafetyCheckForApplicationProcessed;


            Common::TraceEventWriter<std::wstring, Common::TimeSpan, NodePeriodicTaskCounts> NodePeriodicTaskEnd;
            Common::TraceEventWriter<std::wstring, Common::TimeSpan, ServicePeriodicTaskCounts> ServicePeriodicTaskEnd;
            Common::TraceEventWriter<std::wstring, Common::TimeSpan, size_t, size_t> CreateContextsPeriodicTaskEnd;
            Common::TraceEventWriter<std::wstring, Common::TimeSpan, ServiceContextCounts> ServiceContextsPeriodicTaskEnd;
            Common::TraceEventWriter<std::wstring, Common::TimeSpan, bool> FailoverUnitContextsPeriodicTaskEnd;

            Common::TraceEventWriter<Common::Guid, size_t> Movements;

            EventSource(Common::TraceTaskCodes::Enum id) :

                Activated(id, 4, "Activated", Common::LogLevel::Info, "FM {0} is Activated on node {1}:{2}.", "id", "nodeId", "instanceId"),
                Deactivated(id, 5, "Deactivated", Common::LogLevel::Info, "FM {0} is Deactivated on node {1}:{2}.", "id", "nodeId", "instanceId"),
                Ready(id, 6, "Ready", Common::LogLevel::Info, "FM {0} is Ready on node {1}:{2}. Load time: {3}.", "id", "nodeId", "instanceId", "loadTime"),

                Nodes(id, 7, "Nodes", Common::LogLevel::Info, "Up Nodes: {0} {1}-{2} of {3}", "nodes", "first", "last", "total"),
                Movement(id, 8, "Movement", Common::LogLevel::Info, "Prepare to move FT {0} service {1} [{2}-{3}] partition name {4} role {5} from node {6} to {7}", "id", "service", "low", "high", "partition", "role", "from", "to"),
                SwapPrimary(id, 9, "SwapPrimary", Common::LogLevel::Info, "Prepare to swap primary FT {0} service {1} [{2}-{3}] partition name {4} from node {5} to {6}", "id", "service", "low", "high", "partition", "from", "to"),
                Drop(id, 10, "Drop", Common::LogLevel::Info, "Prepare to drop FT {0} service {1} [{2}-{3}] partition name {4} on node {5} role {6}", "id", "service", "low", "high", "partition", "nodeId", "role"),

                FMReplica(id, 11, "FMReplica", Common::LogLevel::Info, "{1} {2} {3}:{4} {5} {6} {7}/{8}\r\n", "contextSequenceId", "state", "node", "replicaId", "replicaInstance", "serviceUpdateVersion", "version", "lastUpDownTime", "lastUpdated"),
                FMFailoverUnit(id, 12, "FMFailoverUnit", Common::LogLevel::Info, "{1} {2} {3} {4} {5} {6} {7} {8} {9} {10} {11}\r\n{12}", "contextSequenceId", "serviceName", "targetReplicaSetSize", "minReplicaSetSize", "description", "flag", "version", "lookupVersion", "healthSequence", "lsn", "isPersistencePending", "lastUpdated", "replicas"),
                StateMachineAction(id, 13, "StateMachineAction", Common::LogLevel::Info, "{1}\r\n", "contextSequenceId", "action"),
                QueueCounts(id, 14, "QueueCounts", Common::LogLevel::Info, "{0}", "queueCounts"),

                ClusterInPauseState(id, 15, "ClusterInPauseState", Common::LogLevel::Warning, "Cluster is in pause state. UpNodeCount = {0}", "count"),
                ClusterEndPauseState(id, 16, "ClusterEndPauseState", Common::LogLevel::Warning, "Cluster has recovered from pause state. UpNodeCount = {0}", "count"),
                EndpointsUpdated(id, 20, "EndpointUpdate_Endpoint", Common::LogLevel::Info, "Replica on node {1} updated endpoints for {2}:{3} at {4}\r\ns: {5}\r\nr: {6}", "id", "nodeInstance", "replicaId", "instanceId", "tag", "servicelocation", "replicatorEndpoint"),

                PeriodicTaskBegin(id, 21, "TaskBegin_BG", Common::LogLevel::Info, "{0}: {1} periodic task started", "fmId", "task"),
                PeriodicTaskBeginNoise(id, 22, "TaskBeginNoise_BG", Common::LogLevel::Noise, "{0}: {1} periodic task started", "fmId", "task"),
                FTPeriodicTaskEnd(id, 23, "BGTaskEnd_BG", Common::LogLevel::Info, "{0}: FT BackgroundManager periodic task ended: IsEnumerationAborted={1}, Unprocessed={2}, Actions={3}, Duration={4} ms", "fmId", "isEnumertionAborted", "failed", "actions", "duration"),

                BackgroundEnumerationAborted(id, 24, "BGEnumAbort_BG", Common::LogLevel::Info, "Background enumeration aborted: IsActive={0}, IsRescheduled={1}, Actions={2}", "isActive", "isRescheduled", "actions"),
                BackgroundThreadStart(id, 25, "BGThreadStart_BG", Common::LogLevel::Info, "Background thread started"),
                BackgroundThreadEndStatistics(id, 26, "BGThreadEndStat_BG", Common::LogLevel::Info, "Background thread ended: Enumerated={0}, Unprocessed={1}, AsyncCommits={2}, AsyncCompleted={3}, IsEnumerationCompleted={4}, IsThrottledThread={5}", "enumerated", "unprocessedFailoverUnits", "asyncCommits", "asyncCompleted", "IsEnumerationCompleted", "isThrottledThread"),
                BackgroundThreadIterationEnd(id, 27, "BGThreadIterationEnd_BG", Common::LogLevel::Info, "Background iteration completed: Enumerated={0}, Unprocessed={1}, AsyncCommits={2}, IsEnumerationAborted={3}", "enumerated", "unprocessedFailoverUnits", "asyncCommits", "IsEnumerationAborted"),
                BackgroundThreadContextCompleted(id, 28, "BGThreadContextCompleted_BG", Common::LogLevel::Info, "Context {0}: IsCompleted={1}", "contextId", "isCompleted"),

                FailoverUnitCounts(id, 29, "FailoverUnitCounts", Common::LogLevel::Info, "{0}\r\nIsContextComplete={1}", "failoverUnitCounts", "isContextComplete"),
                PromoteSecondary(id, 30, "PromoteSecondary", Common::LogLevel::Info, "Prepare to promote FT {0} service {1} [{2}-{3}] partition name {4} on node {5}", "id", "service", "low", "high", "partition", "nodeId"),
                VoidMovement(id, 31, "VoidMovement", Common::LogLevel::Info, "Prepare to cancel upgrade flags for FT {0} service {1} [{2}-{3}] partition name {4} on node {5}", "id", "service", "low", "high", "partition", "nodeId"),
                PlbFunctionCallSlow(id, 32, "PlbFunctionCall", Common::LogLevel::Warning, "PLB function {0} took {1} ms", "id", "duration"),
                CreateApplicationSuccess(id, 33, "CreateApplication_Success", Common::LogLevel::Info, "Application added: {1}\r\nCommit Duration = {2} ms", "id", "applicationInfo", "commitDuration"),
                AppRemoved(id, 34, "Removed_AppUpdate", Common::LogLevel::Info, "Application removed: {1}\r\nCommit Duration = {2} ms, PLB Duration = {3} ms", "id", "applicationInfo", "commitDuration", "plbDuration"),

                AppUpgradeInProgress(id, 35, "InProgress_AppUpgrade", Common::LogLevel::Info, "Upgrade for {0} on domain {1} still in progress: IsContextCompleted={2}, Unprocessed={3}, Ready={4}, Pending={5}, Waiting={6}, Cancel={7}, NotReady={8}, PLBSafetyCheckStatus={9}", "id", "upgradeDomain", "isContextCompleted", "unprocessedFailoverUnitCount", "readyCount", "pendingCount", "waitingCount", "cancelCount", "notReadyCount", "isPLBSafetyCheckDone"),
                FabricUpgradeInProgress(id, 36, "InProgress_FabricUpgrade", Common::LogLevel::Info, "FabricUpgrade on domain {0} still in progress: IsContextCompleted={1}, Unprocessed={2}, Ready={3}, Pending={4}, Waiting={5}, Cancel={6}, NotReady={7}, IsSafeToUpgrade={8}", "upgradeDomain", "isContextCompleted", "unprocessedFailoverUnitCount", "readyCount", "pendingCount", "waitingCount", "cancelCount", "notReadyCount", "isSafeToUpgrade"),

                OnQueueFull(id, 37, "QueueFull_Failure", Common::LogLevel::Warning, "Ignoring message because {0} is full. QueueSize: {1}", "queueName", "queueSize"),
                OnQueueTimeout(id, 38, "Timeout_Failure", Common::LogLevel::Warning, "[{0}] Request timed out. Action: {1}", "messageId", "action"),
                
                PLBSafetyCheckForApplicationReceived(id, 39, "PLBSafetyCheckForApplicationReceived", Common::LogLevel::Info, "Received PLB safety check for app {0}", "appId"),
                PLBSafetyCheckForApplicationProcessed(id, 40, "PLBSafetyCheckForApplicationProcessed", Common::LogLevel::Info, "Processed PLB safety check for app {0} with error {1}", "appId", "error"),

                NodePeriodicTaskEnd(id, 41, "BGTaskEnd_BGEndNode", Common::LogLevel::Info, "{0}: Node periodic task ended in {1}:\r\n{2}", "fmId", "duration", "nodePeriodicTaskCounts"),
                ServicePeriodicTaskEnd(id, 42, "BGTaskEnd_BGEndService", Common::LogLevel::Info, "{0}: Service periodic task ended in {1}:\r\n{2}", "fmId", "duration", "servicePeriodicTaskCounts"),
                CreateContextsPeriodicTaskEnd(id, 43, "BGTaskEnd_BGEndCreateContexts", Common::LogLevel::Info, "{0}: Periodic task create contexts ended in {1}: Contexts={2}, ApplicationUpgradeContexts={3}", "fmId", "duration", "contexts", "applicationUpgradeContexts"),
                ServiceContextsPeriodicTaskEnd(id, 44, "BGTaskEnd_BGEndServiceContexts", Common::LogLevel::Info, "{0}: Periodic task create service contexts ended in {1}: {2}", "fmId", "duration", "serviceContextCounts"),
                FailoverUnitContextsPeriodicTaskEnd(id, 45, "BGTaskEnd_BGEndFTContexts", Common::LogLevel::Info, "{0}: Periodic task create failover unit contexts ended in {1}.", "fmId", "duration", "addedHealthReportContext"),

                Movements(id, 46, "Movements", Common::LogLevel::Info, "{0}: Processing {1} movements", "id", "count")

                // Event id limit: 49
            {
            }
        };
    }
}
