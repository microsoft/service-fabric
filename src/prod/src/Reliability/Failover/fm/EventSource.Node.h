// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class NodeEventSource
        {
        public:

            Common::TraceEventWriter<Federation::NodeId, uint64> NodeAddedToGFUM;
            Common::TraceEventWriter<Federation::NodeId, uint64> NodeRemovedFromGFUM;
            
            Common::TraceEventWriter<std::wstring, uint64> NodeUp;
            Common::TraceEventWriter<std::wstring, uint64> NodeDown;
            Common::TraceEventWriter<Common::Guid, std::wstring, uint64, Common::DateTime> NodeUpOperational;
            Common::TraceEventWriter<Common::Guid, std::wstring, uint64, Common::DateTime> NodeDownOperational;
            Common::TraceEventWriter<std::wstring, uint64> NodeStateRemoved;

            Common::TraceEventWriter<std::wstring, size_t, size_t, FailoverUnitId, bool> DeactivateNodeStatus;
            Common::TraceEventWriter<Federation::NodeId, Common::ErrorCode> DeactivateNodeFailure;
            Common::TraceEventWriter<Federation::NodeId> UpdateNodeSequenceNumberStale;
            Common::TraceEventWriter<Federation::NodeId> PartitionNotificationSequenceNumberStale;

            Common::TraceEventWriter<NodeCounts> NodeCounts;
            Common::TraceEventWriter<Common::Guid, std::wstring, uint64, std::wstring, NodeDeactivationIntent::Trace> DeactivateNodeStartOperational;
            Common::TraceEventWriter<Common::Guid, std::wstring, uint64, NodeDeactivationIntent::Trace, std::wstring, Common::DateTime> DeactivateNodeCompletedOperational;
            Common::TraceEventWriter<Common::Guid, std::wstring, std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring> NodeAddedOperational;
            Common::TraceEventWriter<Common::Guid, std::wstring, std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring> NodeRemovedOperational;

            NodeEventSource(Common::TraceTaskCodes::Enum id) :

                NodeAddedToGFUM(id, 160, "NodeAddedToGFUM", Common::LogLevel::Info, "Node {0}:{1} is added to GFUM.", "nodeId", "instanceId"),
                NodeRemovedFromGFUM(id, 161, "NodeRemovedFromGFUM", Common::LogLevel::Info, "Node {0}:{1} is removed from GFUM.", "nodeId", "instanceId"),

                NodeUp(id, 162, "_Nodes_NodeUp", Common::LogLevel::Info, Common::TraceChannelType::Debug, Common::TraceKeywords::ForQuery, "{0}:{1} is up", "id", "dca_instance"),
                NodeDown(id, 163, "_Nodes_NodeDown", Common::LogLevel::Info, Common::TraceChannelType::Debug, Common::TraceKeywords::ForQuery, "{0}:{1} is down", "id", "dca_instance"),
                NodeStateRemoved(id, 164, "_Nodes_DEL_NodeStateRemoved", Common::LogLevel::Info, Common::TraceChannelType::Debug, Common::TraceKeywords::ForQuery, "{0}:{1} state is removed", "id", "dca_instance"),
                
                DeactivateNodeStatus(id, 165, "Status_DeactivateNode", Common::LogLevel::Info, "DeactivateNode {0}: Up={1}, Unprocessed={2}, FailoverUnitId={3}, IsContextCompleted={4}", "id", "upReplicaCount", "unprocessedFailoverUnitCount", "failoverUnitId", "isContextCompleted"),
                DeactivateNodeFailure(id, 166, "Failure_DeactivateNode", Common::LogLevel::Warning, "Deactivation of node {0} failed with {1}.", "id", "error"),
                UpdateNodeSequenceNumberStale(id, 167, "Stale_UpdateNodeSequenceNumber", Common::LogLevel::Info, "Discarding stale UpdateNodeSequenceNumber request for node {0}", "nodeInstance"),
                PartitionNotificationSequenceNumberStale(id, 168, "Stale_PartitionNotificationSequenceNumber", Common::LogLevel::Info, "Discarding stale PartitionNotification request for node {0}", "nodeInstance"),

                NodeCounts(id, 169, "NodeCounts", Common::LogLevel::Info, "{0}", "nodeCounts"),
                
                DeactivateNodeCompletedOperational(Common::ExtendedEventMetadata::Create(L"NodeDeactivateCompleted", OperationalStateTransitionCategory), id, 170, "_NodesOps_DeactivateNodeCompletedOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "Node Deactivation complete. NodeName : {1}, NodeInstance: {2}, EffectiveDeactivateIntent: {3}, AllBatchIdWithDeactivateIntent: {4}, StartTime: {5}", "eventInstanceId", "nodeName", "nodeInstance", "effectiveDeactivateIntent", "batchIdsWithDeactivateIntent", "startTime"),
                NodeUpOperational(Common::ExtendedEventMetadata::Create(L"NodeUp", OperationalStateTransitionCategory), id, 171, "_NodesOps_NodeUpOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "Node is Up. NodeName : {1}, NodeInstance: {2}, LastNodeDownAt: {3}", "eventInstanceId", "nodeName", "nodeInstance", "lastNodeDownAt"),
                NodeDownOperational(Common::ExtendedEventMetadata::Create(L"NodeDown", OperationalStateTransitionCategory), id, 172, "_NodesOps_NodeDownOperational", Common::LogLevel::Error, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "Node is Down. NodeName : {1}, NodeInstance: {2}, LastNodeUpAt: {3}", "eventInstanceId", "nodeName", "nodeInstance", "lastNodeUpAt"),

                // These traces are very low volume and only written when a new Node shows up for the first time, or when it is removed. We capture as much details about the node in these traces and other traces can reference these.
                NodeAddedOperational(Common::ExtendedEventMetadata::Create(L"NodeAddedToCluster", OperationalStateTransitionCategory), id, 173, "_NodesOps_NodeAddedOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "New Node Added. NodeName : {1}, NodeId: {2}, NodeInstance: {3}, NodeType: {4}, FabricVersion: {5}, IpAddressOrFQDN: {6}, NodeCapacities: {7}", "eventInstanceId", "nodeName", "nodeId", "nodeInstance", "nodeType", "fabricVersion", "ipAddressOrFQDN", "nodeCapacities"),
                NodeRemovedOperational(Common::ExtendedEventMetadata::Create(L"NodeRemovedFromCluster", OperationalStateTransitionCategory), id, 174, "_NodesOps_NodeRemovedOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "Node Removed. NodeName : {1}, NodeId: {2}, NodeInstance: {3}, NodeType : {4}, FabricVersion: {5}, IpAddressOrFQDN: {6}, NodeCapacities: {7}", "eventInstanceId", "nodeName", "nodeId", "nodeInstance", "nodeType", "fabricVersion", "ipAddressOrFQDN", "nodeCapacities"),
                DeactivateNodeStartOperational(Common::ExtendedEventMetadata::Create(L"NodeDeactivateStarted", OperationalStateTransitionCategory), id, 175, "_NodesOps_DeactivateNodeStartOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, Common::TraceKeywords::Default, "Node Deactivation Started. NodeName : {1}, NodeInstance: {2}, BatchId: {3}, DeactivationIntent: {4}", "eventInstanceId", "nodeName", "nodeInstance", "batchId", "deactivateIntent")
                // Event id limit: 220
            {
            }

        };
    }
}
