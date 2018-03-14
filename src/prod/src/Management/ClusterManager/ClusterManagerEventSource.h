// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {

#define DECLARE_CM_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define CM_TRACE(trace_name, trace_id, trace_level, ...) \
        STRUCTURED_TRACE(trace_name, ClusterManager, trace_id, trace_level, __VA_ARGS__),

#define DECLARE_CM_QUERY_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, std::wstring, uint64, int32, __VA_ARGS__);

#define CM_QUERY_TRACE(table_name, trace_name, trace_id, trace_level, trace_format, ...) \
        STRUCTURED_QUERY_TRACE(table_name, trace_name, ClusterManager, trace_id, trace_level, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), trace_format, "id", "dca_instance", "dca_version", __VA_ARGS__),

#define DECLARE_CM_DEL_QUERY_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, std::wstring, uint64, int32, __VA_ARGS__);

#define CM_DEL_QUERY_TRACE(table_name, trace_name, trace_id, trace_level, trace_format, ...) \
        STRUCTURED_DEL_QUERY_TRACE(table_name, trace_name, ClusterManager, trace_id, trace_level, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), trace_format, "id", "dca_instance", "dca_version", __VA_ARGS__),

#define DECLARE_CM_OPERATIONAL_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define CM_OPERATIONAL_TRACE(table_prefix, trace_name, trace_id, trace_level, ...) \
        STRUCTURED_OPERATIONAL_TRACE(table_prefix, trace_name, ClusterManager, trace_id, trace_level, __VA_ARGS__),

        class ClusterManagerEventSource
        {
            BEGIN_STRUCTURED_TRACES( ClusterManagerEventSource )

            // General Request Processing
            //
            CM_TRACE( RequestValidationFailed, 10, Info, "{0} invalid ({1}) message id = {2} reason = {3}", "prId", "action", "messageId", "reason" )
            CM_TRACE( RequestValidationFailed2, 11, Info, "{0} invalid ({1}) message id = {2} reason = {3}", "ractId", "action", "messageId", "reason" )
            CM_TRACE( RequestReceived, 15, Noise, "{0} receive ({1}) message id = {2} timeout = {3}", "ractId", "action", "messageId", "timeout" )
            CM_TRACE( RequestRejected, 20, Info, "{0} reject message id = {1} error = {2}", "ractId", "messageId", "error" )
            CM_TRACE( RequestRetry, 25, Noise, "{0} retry message id = {1} delay = {2} error = {3}", "ractId", "messageId", "delay", "error" )
            CM_TRACE( RequestSucceeded, 30, Noise, "{0} success message id = {1}", "ractId", "messageId" )
            CM_TRACE( RequestFailed, 35, Info, "{0} fail message id = {1} error = {2}", "ractId", "messageId", "error" )

            // Replica Lifecycle
            //
            CM_TRACE( ReplicaOpen, 40, Info, "{0} open node = {1} location = {2} ", "prId", "node", "location" )
            CM_TRACE( ReplicaChangeRole, 45, Info, "{0} change role = {1}", "prId", "role" )
            CM_TRACE( ReplicaClose, 50, Info, "{0} close", "prId" )
            CM_TRACE( ReplicaAbort, 55, Info, "{0} abort", "prId" )

            // Image Builder Proxy
            //
            CM_TRACE( ImageBuilderCreating, 60, Noise, "[{0}] args = '{1}' timeout = {2} buffer = {3}", "node", "args", "timeout", "timeoutbuffer" )
            CM_TRACE( ImageBuilderCreated, 65, Info, "[{0}] handle = {1} process id = {2}", "node", "handle", "processId" )
            CM_TRACE( ImageBuilderFailed, 70, Info, "[{0}] error = {1} message = '{2}'", "node", "error", "message" )
            CM_TRACE( ImageBuilderParseAppTypeInfo, 75, Noise, "[{0}] input = '{1}'", "node", "input" )
            CM_TRACE( ImageBuilderParseFabricVersion, 76, Noise, "[{0}] input = '{1}'", "node", "input" )
            CM_TRACE( ImageBuilderParseFabricUpgradeResult, 77, Noise, "[{0}] input = '{1}'", "node", "input" )

            // RolloutManager
            //
            CM_TRACE( RolloutManagerStart, 85, Info, "{0} start ({1})", "prId", "state" )
            CM_TRACE( RolloutManagerRecovery, 90, Info, "{0} recovering ({1})", "prId", "state" )
            CM_TRACE( RolloutManagerActive, 91, Info, "{0} active", "prId" )

            // Upgrade Processing
            //
            CM_TRACE( UpgradeStart, 95, Info, "{0} upgrade start: {1}", "ractId", "upgradeContext" )
            CM_TRACE( UpgradeInterruption, 100, Info, "{0} upgrade interruption: {1}", "ractId", "upgradeContext" )
            CM_TRACE( UpgradeRequest, 105, Info, "{0} [{1}] upgrade request: {2}", "ractId", "context", "request" )
            CM_TRACE( UpgradeReply, 110, Info, "{0} upgrade reply: {1}", "ractId", "reply" )
            CM_TRACE( UpgradeComplete, 115, Info, "{0} upgrade complete: {1}", "ractId", "upgradeContext" )
            CM_TRACE( RollbackStart, 119, Info, "{0} rolling back: {1}", "ractId", "upgradeContext" )
            CM_TRACE( RollbackComplete, 120, Info, "{0} rollback complete: {1}", "ractId", "upgradeContext" )
            
            // Fabric Client Proxy
            //
            CM_TRACE( InnerCreateName, 125, Noise, "{0} name = {1}", "ractId", "name" )
            CM_TRACE( InnerDeleteName, 130, Noise, "{0} name = {1}", "ractId", "name" )
            CM_TRACE( InnerCreateService, 135, Noise, "{0} service = {1} package = {2} instance = {3}", "ractId", "service", "package", "instance" )
            CM_TRACE( InnerUpdateService, 138, Noise, "{0} name = {1} description = {2}", "ractId", "name", "description" )
            CM_TRACE( InnerDeleteService, 140, Noise, "{0} service = {1}", "ractId", "description" )
            CM_TRACE( InnerGetServiceDescription, 141, Noise, "{0} service = {1}", "ractId", "name" )
            CM_TRACE( InnerPutProperty, 145, Noise, "{0} name = {1} prop = {2}", "ractId", "name", "property" )
            CM_TRACE( InnerDeleteProperty, 150, Noise, "{0} name = {1} prop = {2}", "ractId", "name", "property" )
            CM_TRACE( InnerSubmitPropertyBatch, 151, Noise, "{0} batch = {1}", "ractId", "batch" )

            CM_TRACE( InnerDeactivateNodesBatch, 160, Noise, "{0} batchId = {1}", "ractId", "batchId" )
            CM_TRACE( InnerRemoveNodeDeactivations, 161, Noise, "{0} batchId = {1}", "ractId", "batchId" )
            CM_TRACE( InnerGetNodeDeactivationStatus, 162, Noise, "{0} batchId = {1}", "ractId", "batchId" )
            CM_TRACE( InnerNodeStateRemoved, 163, Noise, "{0} node = {1}/{2}", "ractId", "nodeName", "nodeId" )
            CM_TRACE( InnerReportStartTaskSuccess, 164, Noise, "{0} task = {1}", "ractId", "taskInstance" )
            CM_TRACE( InnerReportFinishTaskSuccess, 165, Noise, "{0} task = {1}", "ractId", "taskInstance" )
            CM_TRACE( InnerReportTaskFailure, 166, Noise, "{0} task = {1}", "ractId", "taskInstance" )

            CM_QUERY_TRACE( Applications, ApplicationCreated, 170, Info, "{3} Application {0} Created: type = {4} version = {5} manifestId = {6}", "ractId", "typename", "typeversion", "manifestid" )
            CM_QUERY_TRACE( Applications, ApplicationUpgradeStart, 171, Info, "{3} Application {0} Upgrading: target = {4} current_manifestId = {5} target_manifestid = {6}", "ractId", "targetversion", "currentmanifestid", "targetmanifestid" )
            CM_QUERY_TRACE( Applications, ApplicationUpgradeComplete, 172, Info, "{3} Application {0} Upgraded: target = {4} current_manifestId = {5} target_manifestid = {6}", "ractId", "targetversion", "currentmanifestid", "targetmanifestid" )
            CM_QUERY_TRACE( Applications, ApplicationUpgradeRollback, 173, Info, "{3} Application {0} Rolling Back: target = {4} current_manifestId = {5} target_manifestid = {6}", "ractId", "targetversion", "currentmanifestid", "targetmanifestid" )
            CM_QUERY_TRACE( Applications, ApplicationUpgradeRollbackComplete, 174, Info, "{3} Application {0} Rolled Back: target = {4} current_manifestId = {5} target_manifestid = {6}", "ractId", "targetversion", "currentmanifestid", "targetmanifestid" )
            CM_DEL_QUERY_TRACE( Applications, ApplicationDeleted, 175, Info, "{3} Application {0} Deleted, manifestId = {4}", "ractId", "manifestid" )
            CM_QUERY_TRACE( Applications, ApplicationUpgradeDomainComplete, 176, Info, "{3} Application {0} Upgrade Domain Completed: target = {4} domains = {5}", "ractId", "targetversion", "upgradedomains" )

            CM_OPERATIONAL_TRACE( Applications, ApplicationCreatedOperational, 180, Info, "Application created: Application = {0}, Application Type = {1}, Application Type Version = {2}", "applicationName", "applicationTypeName", "applicationTypeVersion" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationUpgradeStartOperational, 181, Info, "Application upgrade started: Application = {0}, Application Type = {1}, Target Application Type Version = {2}, Upgrade Type = {3}, Rolling Upgrade Mode = {4}, Failure Action = {5}", "applicationName", "applicationTypeName", "applicationTypeVersion", "upgradeType", "rollingUpgradeMode", "failureAction" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationUpgradeCompleteOperational, 182, Info, "Application upgrade completed: Application = {0}, Application Type = {1}, Target Application Type Version = {2}, Overall Elapsed Time {3}", "applicationName", "applicationTypeName", "applicationTypeVersion", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationUpgradeRollbackOperational, 183, Info, "Application rollback start: Application = {0}, Application Type = {1}, Target Application Type Version = {2}, Failure Reason = {3}, Overall Elapsed Time = {4}", "applicationName", "applicationTypeName", "applicationTypeVersion", "failureReason", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationUpgradeRollbackCompleteOperational, 184, Info, "Application rollback completed: Application = {0}, Application Type = {1}, Target Application Type Version = {2}, Failure Reason = {3}, Overall Elapsed Time = {4}", "applicationName", "applicationTypeName", "applicationTypeVersion", "failureReason", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationDeletedOperational, 185, Info, "Application deleted: Application = {0}, Application Type = {1}", "applicationName", "applicationTypeName" )
            CM_OPERATIONAL_TRACE( Applications, ApplicationUpgradeDomainCompleteOperational, 186, Info, "Application upgrade domain completed: Application = {0}, Application Type = {1}, Target Application Type Version = {2}, Upgrade State = {3}, Upgrade Domains = {4}, Upgrade Domain Elapsed Time = {5}",  "applicationName", "applicationTypeName", "applicationTypeVersion", "upgradeState", "upgradeDomains", "upgradeDomainElapsedTime" )

            CM_OPERATIONAL_TRACE( Cluster, ClusterUpgradeStartOperational, 187, Info, "Cluster upgrade started: Target Cluster Version = {0}, Upgrade Type = {1}, Rolling Upgrade Mode = {2}, Failure Action = {3}", "clusterVersion", "upgradeType", "rollingUpgradeMode", "failureAction" )
            CM_OPERATIONAL_TRACE( Cluster, ClusterUpgradeCompleteOperational, 188, Info, "Cluster upgrade completed: Target Cluster Version = {0}, Overall Elapsed Time = {1}", "clusterVersion", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Cluster, ClusterUpgradeRollbackOperational, 189, Info, "Cluster rollback start: Target Cluster Version = {0}, Failure Reason = {1}, Overall Elapsed Time = {2}", "clusterVersion", "failureReason", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Cluster, ClusterUpgradeRollbackCompleteOperational, 190, Info, "Cluster rollback completed: Target Cluster Version = {0}, Failure Reason = {1}, Overall Elapsed Time = {2}", "clusterVersion", "failureReason", "overallUpgradeElapsedTime" )
            CM_OPERATIONAL_TRACE( Cluster, ClusterUpgradeDomainCompleteOperational, 191, Info, "Cluster upgrade domain completed: Target Cluster Version = {0}, Upgrade State = {1}, Upgrade Domains = {2}, Upgrade Domain Elapsed Time = {3}", "clusterVersion", "upgradeState", "upgradeDomains", "upgradeDomainElapsedTime" )


            END_STRUCTURED_TRACES

            // General Request Processing
            //
            DECLARE_CM_TRACE( RequestValidationFailed, Store::PartitionedReplicaId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_CM_TRACE( RequestValidationFailed2, Store::ReplicaActivityId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_CM_TRACE( RequestReceived, Store::ReplicaActivityId, std::wstring, Transport::MessageId, Common::TimeSpan )
            DECLARE_CM_TRACE( RequestRejected, Store::ReplicaActivityId, Transport::MessageId, Common::ErrorCode )
            DECLARE_CM_TRACE( RequestRetry, Store::ReplicaActivityId, Transport::MessageId, Common::TimeSpan, Common::ErrorCode )
            DECLARE_CM_TRACE( RequestSucceeded, Store::ReplicaActivityId, Transport::MessageId )
            DECLARE_CM_TRACE( RequestFailed, Store::ReplicaActivityId, Transport::MessageId, Common::ErrorCode )
            
            // Replica Lifecycle	
            //
            DECLARE_CM_TRACE( ReplicaOpen, Store::PartitionedReplicaId, Federation::NodeInstance, SystemServices::SystemServiceLocation )
            DECLARE_CM_TRACE( ReplicaChangeRole, Store::PartitionedReplicaId, std::wstring )
            DECLARE_CM_TRACE( ReplicaClose, Store::PartitionedReplicaId )
            DECLARE_CM_TRACE( ReplicaAbort, Store::PartitionedReplicaId )
            
            // Image Builder Proxy
            //
            DECLARE_CM_TRACE( ImageBuilderCreating, Federation::NodeInstance, std::wstring, Common::TimeSpan, Common::TimeSpan )
            DECLARE_CM_TRACE( ImageBuilderCreated, Federation::NodeInstance, int64, int )
            DECLARE_CM_TRACE( ImageBuilderFailed, Federation::NodeInstance, Common::ErrorCode, std::wstring )
            DECLARE_CM_TRACE( ImageBuilderParseAppTypeInfo, Federation::NodeInstance, std::wstring )
            DECLARE_CM_TRACE( ImageBuilderParseFabricVersion, Federation::NodeInstance, std::wstring )
            DECLARE_CM_TRACE( ImageBuilderParseFabricUpgradeResult, Federation::NodeInstance, std::wstring )

            // RolloutManager
            //
            DECLARE_CM_TRACE( RolloutManagerStart, Store::PartitionedReplicaId, RolloutManagerState::Trace )
            DECLARE_CM_TRACE( RolloutManagerRecovery, Store::PartitionedReplicaId, RolloutManagerState::Trace )
            DECLARE_CM_TRACE( RolloutManagerActive, Store::PartitionedReplicaId )

            // Upgrade Processing
            //
            DECLARE_CM_TRACE( UpgradeStart, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( UpgradeInterruption, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( UpgradeRequest, Store::ReplicaActivityId, std::wstring, std::wstring )
            DECLARE_CM_TRACE( UpgradeReply, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( UpgradeComplete, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( RollbackStart, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( RollbackComplete, Store::ReplicaActivityId, std::wstring )

            // Fabric Client Proxy
            //
            DECLARE_CM_TRACE( InnerCreateName, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( InnerDeleteName, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( InnerCreateService, Store::ReplicaActivityId, std::wstring, std::wstring, uint64 )
            DECLARE_CM_TRACE( InnerUpdateService, Store::ReplicaActivityId, Common::NamingUri, Naming::ServiceUpdateDescription)
            DECLARE_CM_TRACE( InnerDeleteService, Store::ReplicaActivityId, ServiceModel::DeleteServiceDescription)
            DECLARE_CM_TRACE( InnerGetServiceDescription, Store::ReplicaActivityId, Common::NamingUri )
            DECLARE_CM_TRACE( InnerPutProperty, Store::ReplicaActivityId, std::wstring, std::wstring )
            DECLARE_CM_TRACE( InnerDeleteProperty, Store::ReplicaActivityId, std::wstring, std::wstring )
            DECLARE_CM_TRACE( InnerDeactivateNodesBatch, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( InnerRemoveNodeDeactivations, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( InnerGetNodeDeactivationStatus, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_TRACE( InnerNodeStateRemoved, Store::ReplicaActivityId, std::wstring, Federation::NodeId )
            DECLARE_CM_TRACE( InnerReportStartTaskSuccess, Store::ReplicaActivityId, TaskInstance )
            DECLARE_CM_TRACE( InnerReportFinishTaskSuccess, Store::ReplicaActivityId, TaskInstance )
            DECLARE_CM_TRACE( InnerReportTaskFailure, Store::ReplicaActivityId, TaskInstance )
            DECLARE_CM_TRACE( InnerSubmitPropertyBatch, Store::ReplicaActivityId, std::wstring )

            // Queryable Application Lifecycle
            //
            DECLARE_CM_QUERY_TRACE( ApplicationCreated, Store::ReplicaActivityId, std::wstring, std::wstring, std::wstring )
            DECLARE_CM_QUERY_TRACE( ApplicationUpgradeStart, Store::ReplicaActivityId, std::wstring, std::wstring, std::wstring )
            DECLARE_CM_QUERY_TRACE( ApplicationUpgradeComplete, Store::ReplicaActivityId, std::wstring, std::wstring, std::wstring )
            DECLARE_CM_QUERY_TRACE( ApplicationUpgradeRollback, Store::ReplicaActivityId, std::wstring, std::wstring, std::wstring)
            DECLARE_CM_QUERY_TRACE( ApplicationUpgradeRollbackComplete, Store::ReplicaActivityId, std::wstring, std::wstring, std::wstring)
            DECLARE_CM_DEL_QUERY_TRACE( ApplicationDeleted, Store::ReplicaActivityId, std::wstring )
            DECLARE_CM_QUERY_TRACE( ApplicationUpgradeDomainComplete, Store::ReplicaActivityId, std::wstring, std::wstring)

            // Operational Application Lifecycle
            //
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationCreatedOperational, std::wstring, std::wstring, std::wstring )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationUpgradeStartOperational, std::wstring, std::wstring, std::wstring, ServiceModel::UpgradeType::Trace, ServiceModel::RollingUpgradeMode::Trace, ServiceModel::MonitoredUpgradeFailureAction::Trace )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationUpgradeCompleteOperational, std::wstring, std::wstring, std::wstring, Common::TimeSpan )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationUpgradeRollbackOperational, std::wstring, std::wstring, std::wstring, Management::ClusterManager::UpgradeFailureReason::Trace, Common::TimeSpan )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationUpgradeRollbackCompleteOperational, std::wstring, std::wstring, std::wstring, Management::ClusterManager::UpgradeFailureReason::Trace, Common::TimeSpan )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationDeletedOperational, std::wstring, std::wstring )
            DECLARE_CM_OPERATIONAL_TRACE( ApplicationUpgradeDomainCompleteOperational, std::wstring, std::wstring, std::wstring, Management::ClusterManager::ApplicationUpgradeState::Trace, std::wstring, Common::TimeSpan )

            // Fabric Upgrade Lifecycle
            //
            DECLARE_CM_OPERATIONAL_TRACE( ClusterUpgradeStartOperational, std::wstring, ServiceModel::UpgradeType::Trace, ServiceModel::RollingUpgradeMode::Trace, ServiceModel::MonitoredUpgradeFailureAction::Trace )
            DECLARE_CM_OPERATIONAL_TRACE( ClusterUpgradeCompleteOperational, std::wstring, Common::TimeSpan)
            DECLARE_CM_OPERATIONAL_TRACE( ClusterUpgradeRollbackOperational, std::wstring, Management::ClusterManager::UpgradeFailureReason::Trace, Common::TimeSpan )
            DECLARE_CM_OPERATIONAL_TRACE( ClusterUpgradeRollbackCompleteOperational, std::wstring, Management::ClusterManager::UpgradeFailureReason::Trace, Common::TimeSpan )
            DECLARE_CM_OPERATIONAL_TRACE( ClusterUpgradeDomainCompleteOperational, std::wstring, Management::ClusterManager::FabricUpgradeState::Trace, std::wstring, Common::TimeSpan )

            static Common::Global<ClusterManagerEventSource> Trace;
        };

        typedef ClusterManagerEventSource CMEvents; 
    }
}
