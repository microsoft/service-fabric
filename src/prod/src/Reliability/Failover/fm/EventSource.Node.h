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
            Common::TraceEventWriter<Common::Guid, std::wstring, uint64, NodeDeactivationIntent::Trace, Common::DateTime> DeactivateNodeCompletedOperational;
            
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
                
                DeactivateNodeCompletedOperational(id, 170, "_NodesOperational_DeactivateNodeCompletedOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Default, "Node Deactivation complete. NodeName : {1}, NodeInstance: {2}, EffectiveDeactivateIntent: {3}, StartTime: {4}", "eventInstanceId", "nodeName", "nodeInstance", "effectiveIntent", "startTime"),
                NodeUpOperational(id, 171, "_NodesOperational_NodeUpOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Default, "Node is Up. NodeName : {1}, NodeInstance: {2}, LastNodeDownAt: {3}", "eventInstanceId", "nodeName", "nodeInstance", "lastNodeDownAt"),
                NodeDownOperational(id, 172, "_NodesOperational_NodeDownOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Default, "Node is Down. NodeName : {1}, NodeInstance: {2}, LastNodeUpAt: {3}", "eventInstanceId", "nodeName", "nodeInstance", "lastNodeUpAt")

                // Event id limit: 220
            {
            }

        };
    }
}
