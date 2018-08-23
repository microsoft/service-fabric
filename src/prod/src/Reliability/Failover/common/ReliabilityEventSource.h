// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
#define DECLARE_RELIABILITY_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define RELIABILITY_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::Reliability, \
            trace_id, \
            #trace_name "_" trace_type, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ReliabilityEventSource
    {
    public:
        DECLARE_RELIABILITY_STRUCTURED_TRACE(ServiceDescription,
            uint16,
            std::wstring,
            std::wstring,
            std::wstring,
            ServiceModel::ServicePackageVersionInstance,
            int32,
            int32,
            int32,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaDescription,
            uint16,
            std::wstring,
            Federation::NodeInstance,
            int64,
            int64,
            int64,
            int64,
            ServiceModel::ServicePackageVersionInstance);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            Reliability::ReplicaDescription,
            std::wstring,
            std::wstring,
            int32,
            int32,
            uint64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaReplyMessageBody,
            uint16,
            FailoverUnitDescription,
            Reliability::ReplicaDescription,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ConfigurationMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            std::vector<Reliability::ReplicaDescription>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ConfigurationReplyMessageBody,
            uint16,
            FailoverUnitDescription,
            std::vector<Reliability::ReplicaDescription>,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(FailoverUnitMessageBody,
            uint16,
            FailoverUnitDescription);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(FailoverUnitReplyMessageBody,
            uint16,
            FailoverUnitDescription,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaInfo,
            uint16,
            std::wstring,
            std::wstring,
            std::wstring,
            Reliability::ReplicaDescription);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(FailoverUnitInfo,
            uint16,
            Common::Guid,
            int,
            int,
            Reliability::Epoch,
            Reliability::Epoch,
            Reliability::Epoch,
            bool,
            bool, // islocalreplicadeleted
            std::vector<Reliability::ReplicaInfo>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeUpAckMessageBody,
            uint16,
            bool,
            int64,
            Common::FabricVersionInstance,
            std::vector<Reliability::UpgradeDescription>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaUpMessageBody,
            uint16,
            bool,
            bool,
            std::vector<Reliability::FailoverUnitInfo>,
            std::vector<Reliability::FailoverUnitInfo>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(UpgradeDescription,
            uint16,
            ServiceModel::ApplicationUpgradeSpecification);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReplicaListMessageBody,
            uint16,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ReportFaultMessageBody,
            uint16,
            Reliability::FailoverUnitDescription,
            Reliability::ReplicaDescription,
            Reliability::FaultType::Trace,
            Common::ActivityDescription);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(FailoverUnitId,
            uint16,
            Common::Guid);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(DeactivateMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            std::vector<Reliability::ReplicaDescription>,
            bool,
            Reliability::ReplicaDeactivationInfo);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(DoReconfigurationMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            std::vector<Reliability::ReplicaDescription>,
            Common::TimeSpan);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(SystemServiceLocation,
            uint16,
            Federation::NodeInstance,
            Common::Guid,
            int64,
            int64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(StatefulServiceReplicaSetTrace,
            uint16,
            std::wstring,
            std::wstring,
            int64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(StatelessServiceReplicaSetTrace,
            uint16,
            std::wstring,
            int64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeFabricUpgradeReplyMessageBody,
            uint16,
            Common::FabricVersionInstance);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeUpdateServiceReplyMessageBody,
            uint16,
            std::wstring,
            uint64,
            uint64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeActivateRequestMessageBody,
            uint16,
            int64,
            Reliability::FailoverManagerId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeDeactivateRequestMessageBody,
            uint16,
            int64,
            Reliability::FailoverManagerId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeActivateReplyMessageBody,
            uint16,
            int64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeDeactivateReplyMessageBody,
            uint16,
            int64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(NodeUpgradeReplyMessageBody,
            uint16,
            ServiceModel::ApplicationIdentifier,
            uint64,
            Reliability::ReplicaUpMessageBody);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(GetLSNReplyMessageBody,
            uint16,
            FailoverUnitDescription,
            Reliability::ReplicaDescription,
            Reliability::ReplicaDeactivationInfo,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(RAReplicaMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            Reliability::ReplicaDescription,
            Reliability::ServiceDescription,
            Reliability::ReplicaDeactivationInfo);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ActivateMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            std::vector<Reliability::ReplicaDescription>,
            Reliability::ReplicaDeactivationInfo);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginCreateService,
            Common::ActivityId,
            Reliability::ServiceDescription,
            std::vector<Reliability::ConsistencyUnitDescription>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndCreateService,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginUpdateSystemService,
            Common::ActivityId,
            std::wstring,
            Naming::ServiceUpdateDescription);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginUpdateUserService,
            Common::ActivityId,
            Reliability::ServiceDescription);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndUpdateService,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginDeleteService,
            Common::ActivityId,
            std::wstring,
            bool,
            uint64);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndDeleteService,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginQuery,
            Common::ActivityId,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndQuery,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginDeactivateNodes,
            Common::ActivityId,
            std::wstring,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndDeactivateNodes,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginRemoveNodeDeactivations,
            Common::ActivityId,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndRemoveNodeDeactivations,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginGetNodeDeactivationStatus,
            Common::ActivityId,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndGetNodeDeactivationsStatus,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginNodeStateRemoved,
            Common::ActivityId,
            Federation::NodeId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndNodeStateRemoved,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginRecoverPartitions,
            Common::ActivityId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndRecoverPartitions,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginRecoverPartition,
            Common::ActivityId,
            PartitionId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndRecoverPartition,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginRecoverServicePartitions,
            Common::ActivityId,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndRecoverServicePartitions,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginRecoverSystemPartitions,
            Common::ActivityId);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndRecoverSystemPartitions,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(BeginGetServiceDescription,
            Common::ActivityId,
            std::wstring,
            bool);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(EndGetServiceDescription,
            Common::ActivityId,
            Common::ErrorCode);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ServiceTypeInfo,
            uint16,
            ServiceModel::VersionedServiceTypeIdentifier,
            std::wstring);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ServiceTypeNotificationRequestMessageBody,
            uint16,
            std::vector<Reliability::ServiceTypeInfo>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ServiceTypeNotificationReplyMessageBody,
            uint16,
            std::vector<Reliability::ServiceTypeInfo>);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(DeleteReplicaMessageBody,
            uint16,
            FailoverUnitDescription,
            uint64,
            Reliability::ReplicaDescription,
            Reliability::ServiceDescription,
            bool);

        DECLARE_RELIABILITY_STRUCTURED_TRACE(ServiceTableUpdateMessageBody,
            uint16,
            GenerationNumber,
            uint64,
            Common::VersionRangeCollection,
            int64,
            bool);

        ReliabilityEventSource() :
            RELIABILITY_STRUCTURED_TRACE(ServiceDescription,                            4,  "ServiceDescription",                           Info, "{1}:{2}@{3} ,{4}) [{5}]/{6}/{7} {8} ", "contextSequenceId", "serviceName", "serviceType", "applicationName", "packageVersion", "partitionCount", "targetReplicaSetSize", "minReplicaSetSize", "details"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaDescription,                            5,  "ReplicaDescription",                           Info, "{1} {2} {3}:{4} {5} {6} {7}\r\n", "contextSequenceId", "states", "nodeInstance", "replicaId", "replicaInstance", "firstAcknowledgedLSN", "lastAcknowledgedLSN", "packageVersion"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaMessageBody,                            6,  "ReplicaMessageBody",                           Info, "{1} {2}\r\n{3}\r\n{4}:{5}, {6}/{7} {8}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replica", "serviceName", "serviceTypeName", "targetReplicaSetSize", "minReplicaSetSize", "serviceInstance"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaReplyMessageBody,                       7,  "ReplicaReplyMessageBody",                      Info, "{1}\r\n{2}EC: {3}", "contextSequenceId", "failoverUnitDescription", "replica", "errorCode"),
            RELIABILITY_STRUCTURED_TRACE(ConfigurationMessageBody,                      8,  "ConfigurationMessageBody",                     Info, "{1} {2}\r\n{3}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replicas"),
            RELIABILITY_STRUCTURED_TRACE(ConfigurationReplyMessageBody,                 9,  "ConfigurationReplyMessageBody",                Info, "{1}\r\n{2}EC: {3}", "contextSequenceId", "failoverUnitDescription", "replicas", "errorCode"),
            RELIABILITY_STRUCTURED_TRACE(FailoverUnitMessageBody,                       10, "FailoverUnitMessageBody",                      Info, "{1}", "contextSequenceId", "failoverUnitDescription"),
            RELIABILITY_STRUCTURED_TRACE(FailoverUnitReplyMessageBody,                  11, "FailoverUnitReplyMessageBody",                 Info, "{1}\r\nEC: {2}", "contextSequenceId", "failoverUnitDescription", "errorCode"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaInfo,                                   12, "ReplicaInfo",                                  Info, "{1}/{2}/{3} {4}", "contextSequenceId", "pcRole", "icRole", "ccRole", "description"),
            RELIABILITY_STRUCTURED_TRACE(FailoverUnitInfo,                              13, "FailoverUnitInfo",                             Info, "{1} {2} {3} {4}/{5}/{6} {7} {8} \r\n{9}", "contextSequenceId", "failoverUnitId", "targetReplicaSetSize", "minReplicaSetSize", "pcEpoch", "icEpoch", "ccEpoch", "reportFromPrimary", "localReplicaDeleted", "replicaInfo"),
            RELIABILITY_STRUCTURED_TRACE(NodeUpAckMessageBody,                          14, "NodeUpAckMessageBody",                         Info, " {1} {2} {3} {4}\r\n", "contextSequenceId", "isActivated", "seqNum", "fabricVersionInstance", "upgrades"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaUpMessageBody,                          15, "ReplicaUpMessageBody",                         Info, "ReplicaList ,IsLast = {1}, IsFromFMM = {2}):\r\n{3}\r\nDroppedReplicas:\r\n{4}", "contextSequenceId", "isLast", "isFromFMM", "replicaList", "droppedReplicas"),
            RELIABILITY_STRUCTURED_TRACE(UpgradeDescription,                            16, "UpgradeDescription",                           Info, "{1}", "contextSequenceId", "upgradeSpecification"),
            RELIABILITY_STRUCTURED_TRACE(ReplicaListMessageBody,                        17, "ReplicaListMessageBody",                       Info, "{1}", "contextSequenceId", "replicas"),
            RELIABILITY_STRUCTURED_TRACE(ReportFaultMessageBody,                        18, "ReportFaultMessageBody",                       Info, "{1} {2} {3} {4}", "contextSequenceId", "failoverUnitDescription", "replicaDescription", "faultType", "activityDescription"),
            RELIABILITY_STRUCTURED_TRACE(FailoverUnitId,                                19, "FailoverUnitId",                               Info, "{1}\r\n", "contextSequenceId", "ftId"),
            RELIABILITY_STRUCTURED_TRACE(DeactivateMessageBody,                         20, "DeactivateMessageBody",                        Info, "{1} {2}\r\n{3}{4} DeactivationInfo = {5}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replicas", "isForce", "deactivationInfo"),
            RELIABILITY_STRUCTURED_TRACE(SystemServiceLocation,                         22, "SystemServiceLocation",                        Info, "{1}+{2}+{3}+{4}", "contextSequenceId", "node", "partitionId", "replicaId", "replicaInstance"),
            RELIABILITY_STRUCTURED_TRACE(StatefulServiceReplicaSetTrace,                23, "StatefulServiceReplicaSetTrace",               Info, "Primary=[{1}], Secondaries=[{2}], Version={3}", "contextSequenceId", "primary", "secondaries", "version"),
            RELIABILITY_STRUCTURED_TRACE(StatelessServiceReplicaSetTrace,               24, "StatelessServiceReplicaSetTrace",              Info, "Replicas=[{1}], Version={2}", "contextSequenceId", "replicas", "version"),
            RELIABILITY_STRUCTURED_TRACE(NodeFabricUpgradeReplyMessageBody,             25, "NodeFabricUpgradeMessageReplyBody",            Info, "VersionInstance: {1}", "contextSequenceId", "vi"),
            RELIABILITY_STRUCTURED_TRACE(NodeUpdateServiceReplyMessageBody,             26, "NodeUpdateServiceReplyMessageBody",            Info, "{1} {2}:{3}", "contextSequenceId", "name", "instance", "version"),
            RELIABILITY_STRUCTURED_TRACE(NodeActivateRequestMessageBody,                27, "NodeActivateRequestMessageBody",               Info, "seq = {1}. {2}", "contextSequenceId", "sequenceNumber", "sender"),
            RELIABILITY_STRUCTURED_TRACE(NodeDeactivateRequestMessageBody,              28, "NodeDeactivateRequestMessageBody",             Info, "seq = {1}. {2}", "contextSequenceId", "sequenceNumber", "sender"),
            RELIABILITY_STRUCTURED_TRACE(NodeActivateReplyMessageBody,                  29, "NodeActivateReplyMessageBody",                 Info, "{1}", "contextSequenceId", "sequenceNumber"),
            RELIABILITY_STRUCTURED_TRACE(NodeDeactivateReplyMessageBody,                30, "NodeDeactivateReplyMessageBody",               Info, "{1}", "contextSequenceId", "sequenceNumber"),
            RELIABILITY_STRUCTURED_TRACE(NodeUpgradeReplyMessageBody,                   31, "NodeUpgradeReplyMessageBody",                  Info, "ApplicationId: {1} InstanceId: {2}\r\n{3}\r\n", "contextSequenceId", "appId", "instId", "replicas"),
            RELIABILITY_STRUCTURED_TRACE(GetLSNReplyMessageBody,                        32, "GetLSNReplyMessageBody",                       Info, "{1}\r\n{2} {3} EC: {4}", "contextSequenceId", "failoverUnitDescription", "replica", "deactivationInfo", "errorCode"),
            RELIABILITY_STRUCTURED_TRACE(RAReplicaMessageBody,                          33, "RAReplicaMessageBody",                         Info, "{1} {2}\r\n{3}{4} {5}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replica", "service", "replicaDeactivationInfo"),
            RELIABILITY_STRUCTURED_TRACE(ActivateMessageBody,                           34, "ActivateMessageBody",                          Info, "{1} {2}\r\n{3}. DeactivationInfo = {4}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replicas", "replicaDeactivationInfo"),
            RELIABILITY_STRUCTURED_TRACE(BeginCreateService,                            40, "Begin_CreateService",                          Info, "[{0}] Starting CreateService for {1} (cuids={2})", "activityId", "serviceDescription", "consistencyUnitDescriptions"),
            RELIABILITY_STRUCTURED_TRACE(EndCreateService,                              41, "End_CreateService",                            Info, "[{0}] CreateService completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginUpdateSystemService,                      42, "Begin_System_UpdateService",                   Info, "[{0}] Starting UpdateService for {1}: {2}", "activityId", "serviceName", "serviceUpdateDescription"),
            RELIABILITY_STRUCTURED_TRACE(BeginUpdateUserService,                        43, "Begin_User_UpdateService",                     Info, "[{0}] Starting UpdateService for {1}", "activity", "serviceDescription"),
            RELIABILITY_STRUCTURED_TRACE(EndUpdateService,                              44, "End_UpdateService",                            Info, "[{0}] UpdateService completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginDeleteService,                            45, "Begin_DeleteService",                          Info, "[{0}] Starting DeleteService for {1}, IsForce {2} ({3})", "activityId", "serviceName", "isForce", "instanceId"),
            RELIABILITY_STRUCTURED_TRACE(EndDeleteService,                              46, "End_DeleteService",                            Info, "[{0}] DeleteService completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginQuery,                                    47, "Begin_Query",                                  Info, "[{0}] Starting Query for {1}", "activityId", "queryName"),
            RELIABILITY_STRUCTURED_TRACE(EndQuery,                                      48, "End_Query",                                    Info, "[{0}] Query completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginDeactivateNodes,                          49, "Begin_DeactivateNodes",                        Info, "[{0}] Starting DeactivateNodes for {1}: {2}", "activityId", "batchId", "nodeDeactivationIntents"),
            RELIABILITY_STRUCTURED_TRACE(EndDeactivateNodes,                            50, "End_DeactivateNodes",                          Info, "[{0}] DeactivateNodes completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginRemoveNodeDeactivations,                  51, "Begin_RemoveNodeDeactivations",                Info, "[{0}] Starting RemoveNodeDeactivations for {1}", "activityId", "batchId"),
            RELIABILITY_STRUCTURED_TRACE(EndRemoveNodeDeactivations,                    52, "End_RemoveNodeDeactivations",                  Info, "[{0}] RemoveNodeDeactivations completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginGetNodeDeactivationStatus,                53, "Begin_GetNodeDeactivationStatus",              Info, "[{0}] Starting GetNodeDeactivationStatus for {1}", "activityId", "batchId"),
            RELIABILITY_STRUCTURED_TRACE(EndGetNodeDeactivationsStatus,                 54, "End_GetNodeDeactivationStatus",                Info, "[{0}] GetNodeDeactivationStatus completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginNodeStateRemoved,                         55, "Begin_NodeStateRemoved",                       Info, "[{0}] Starting NodeStateRemoved for {1}", "activityId", "nodeId"),
            RELIABILITY_STRUCTURED_TRACE(EndNodeStateRemoved,                           56, "End_NodeStateRemoved",                         Info, "[{0}] NodeStateRemoved completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginRecoverPartitions,                        57, "Begin_RecoverPartitions",                      Info, "[{0}] Starting RecoverPartitions", "activityId"),
            RELIABILITY_STRUCTURED_TRACE(EndRecoverPartitions,                          58, "End_RecoverPartitions",                        Info, "[{0}] RecoverPartitions completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginRecoverPartition,                         59, "Begin_RecoverPartition",                       Info, "[{0}] Starting RecoverPartition for {1}", "activityId", "partitionId"),
            RELIABILITY_STRUCTURED_TRACE(EndRecoverPartition,                           60, "End_RecoverPartition",                         Info, "[{0}] RecoverPartition completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginRecoverServicePartitions,                 61, "Begin_RecoverServicePartitions",               Info, "[{0}] Starting RecoverServicePartitions for {1}", "activityId", "serviceName"),
            RELIABILITY_STRUCTURED_TRACE(EndRecoverServicePartitions,                   62, "End_RecoverServicePartitions",                 Info, "[{0}] RecoverServicePartitions completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginRecoverSystemPartitions,                  63, "Begin_RecoverSystemPartitions",                Info, "[{0}] Starting RecoverSystemPartitions", "activityId"),
            RELIABILITY_STRUCTURED_TRACE(EndRecoverSystemPartitions,                    64, "End_RecoverSystemPartitions",                  Info, "[{0}] RecoverSystemPartitions completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(BeginGetServiceDescription,                    65, "Begin_GetServiceDescription",                  Info, "[{0}] Starting GetServiceDescription for {1} (CUIDs={2})", "activityId", "serviceName", "includeCUIDs"),
            RELIABILITY_STRUCTURED_TRACE(EndGetServiceDescription,                      66, "End_GetServiceDescription",                    Info, "[{0}] GetServiceDescription completed with {1}", "activityId", "result"),
            RELIABILITY_STRUCTURED_TRACE(ServiceTypeInfo,                               67, "ServiceTypeInfo",                              Info, "[ServiceTypeId: {1}. CodePackage: {2}]\r\n", "contextSequenceId", "infoType", "serviceType", "codePackage"),
            RELIABILITY_STRUCTURED_TRACE(ServiceTypeNotificationRequestMessageBody,     68, "ServiceTypeNotificationRequestMessageBody",    Info, "{1}\r\n", "contextSequenceId", "infos"),
            RELIABILITY_STRUCTURED_TRACE(ServiceTypeNotificationReplyMessageBody,       69, "ServiceTypeNotificationReplyMessageBody",      Info, "{1}\r\n", "contextSequenceId", "infos"),
            RELIABILITY_STRUCTURED_TRACE(DoReconfigurationMessageBody,                  70, "DoReconfigurationMessageBody",                 Info, "{1} {2}\r\n{3}{4}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replicas", "phase0Duration"),
            RELIABILITY_STRUCTURED_TRACE(DeleteReplicaMessageBody,                      71, "DeleteReplicaMessageBody",                     Info, "{1} {2}\r\n{3}{4} {5}", "contextSequenceId", "failoverUnitDescription", "sdUpdateVersion", "replica", "service", "isForce"),
            RELIABILITY_STRUCTURED_TRACE(ServiceTableUpdateMessageBody,                 72, "ServiceTableUpdateMessageBody",                Info, "{1} {2} {3} {4} {5}", "contextSequenceId", "generation", "entries", "versionRanges", "endVersion", "isFromFMM")
        {
        }

    public:
        static Common::Global<ReliabilityEventSource> Events;

    };
}
