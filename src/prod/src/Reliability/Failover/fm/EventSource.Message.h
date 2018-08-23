// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class MessageEventSource
        {
        public:
        
            Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, uint64> MessageReceived;

            Common::TraceEventWriter<Federation::NodeId, Common::ActivityId, int64, std::wstring> DeactivateNodeRequest;
            Common::TraceEventWriter<Federation::NodeId, Common::ActivityId, Common::ErrorCode> DeactivateNodeReply;
            Common::TraceEventWriter<Common::ActivityId, Federation::NodeId> NodeStateRemovedRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> NodeStateRemovedReply;
            
            Common::TraceEventWriter<Federation::NodeId, Common::ActivityId, int64> ActivateNodeRequest;
            Common::TraceEventWriter<Federation::NodeId, Common::ActivityId, Common::ErrorCode> ActivateNodeReply;
            
            Common::TraceEventWriter<Common::ActivityId> RecoverPartitionsRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> RecoverPartitionsReply;
            Common::TraceEventWriter<Common::ActivityId, FailoverUnitId> RecoverPartitionRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> RecoverPartitionReply;
            Common::TraceEventWriter<Common::ActivityId, std::wstring> RecoverServicePartitionsRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> RecoverServicePartitionsReply;
            Common::TraceEventWriter<Common::ActivityId> RecoverSystemPartitionsRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> RecoverSystemPartitionsReply;

            Common::TraceEventWriter<Common::ActivityId, std::wstring, uint64> CreateServiceRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> CreateServiceReply;
            Common::TraceEventWriter<Common::ActivityId, std::wstring, uint64, uint64> UpdateServiceRequest;
            Common::TraceEventWriter<Common::ActivityId, std::wstring> UpdateSystemServiceRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> UpdateServiceReply;
            Common::TraceEventWriter<Common::ActivityId, std::wstring, uint64, bool> DeleteServiceRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> DeleteServiceReply;
            Common::TraceEventWriter<Common::ActivityId, std::wstring> ServiceDescriptionRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> ServiceDescriptionReply;
            Common::TraceEventWriter<Common::ActivityId, ServiceModel::ApplicationIdentifier, uint64> CreateApplicationRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> CreateApplicationReply;
            Common::TraceEventWriter<Common::ActivityId, ServiceModel::ApplicationIdentifier, uint64> DeleteApplicationRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> DeleteApplicationReply;
            Common::TraceEventWriter<Transport::MessageId, Federation::NodeInstance, uint64> ServiceTypeEnabledRequest;
            Common::TraceEventWriter<Transport::MessageId, Common::ErrorCode> ServiceTypeEnabledReply;
            Common::TraceEventWriter<Transport::MessageId, Federation::NodeInstance, uint64> ServiceTypeDisabledRequest;
            Common::TraceEventWriter<Transport::MessageId, Common::ErrorCode> ServiceTypeDisabledReply;
            Common::TraceEventWriter<Transport::MessageId, Federation::NodeInstance, uint64> PartitionNotification;
            Common::TraceEventWriter<Transport::MessageId, Common::ErrorCode> PartitionNotificationReply;

            Common::TraceEventWriter<std::wstring, Transport::MessageId, std::wstring, uint64> InvalidMessageDropped;
            
            Common::TraceEventWriter<Common::ActivityId, std::wstring> ResolutionRequest;
            Common::TraceEventWriter<Common::ActivityId, std::wstring, uint64> ResolutionReply;

            Common::TraceEventWriter<Common::ActivityId> QueryRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> QueryReply;
            
            Common::TraceEventWriter<Transport::MessageId, Federation::NodeInstance, ServiceTypeNotificationRequestMessageBody> ServiceTypeNotificationRequest;
            Common::TraceEventWriter<Transport::MessageId, Federation::NodeInstance, ServiceTypeNotificationReplyMessageBody> ServiceTypeNotificationReply;
            Common::TraceEventWriter<ServiceModel::ServiceTypeIdentifier, Common::ErrorCode> ServiceTypeNotificationFailure;
            Common::TraceEventWriter<Common::ActivityId, ServiceModel::ApplicationIdentifier, uint64, uint64> UpdateApplicationRequest;
            Common::TraceEventWriter<Common::ActivityId, Common::ErrorCode> UpdateApplicationReply;

            Common::TraceEventWriter<std::wstring, Federation::NodeInstance, ReplicaListMessageBody> ReplicaDown;
            Common::TraceEventWriter<std::wstring, Federation::NodeInstance, ReplicaListMessageBody> ReplicaDownReply;
            Common::TraceEventWriter<std::wstring, Federation::NodeInstance, size_t, FailoverUnitId> ReplicaDownTimeout;
            
            Common::TraceEventWriter<Common::Guid, ReplicaDescription> FTReplicaDownReceive;
            Common::TraceEventWriter<Common::Guid, Federation::NodeInstance> FTReplicaDownProcess;
            Common::TraceEventWriter<std::wstring, int64> SlowMessageProcess;
            
            MessageEventSource(Common::TraceTaskCodes::Enum id) :
                
                MessageReceived(id, 50, "Receive", Common::LogLevel::Noise, "{0} message received by FM {1} from node {2}:{3}", "id", "fmId", "fromNodeId", "fromInstanceId"),

                DeactivateNodeRequest(id, 51, "Request_DeactivateNode", Common::LogLevel::Info, "[{1}:{2}] Processing DeactivateNode for node {0} with intent {3}.", "id", "activityId", "sequenceNumber", "deactivationIntent"),
                DeactivateNodeReply(id, 52, "Reply_DeactivateNode", Common::LogLevel::Info, "[{1}] DeactivateNode for node {0} completed with error {2}.", "id", "activityId", "error"),
                NodeStateRemovedRequest(id, 53, "Request_NodeStateRemoved", Common::LogLevel::Info, "[{0}] Processing NodeStateRemoved for node {1}.", "activityId", "nodeId"),
                NodeStateRemovedReply(id, 54, "Reply_NodeStateRemoved", Common::LogLevel::Info, "[{0}] NodeStateRemoved completed with error {1}.", "activityId", "error"),
                
                ActivateNodeRequest(id, 55, "Request_ActivateNode", Common::LogLevel::Info, "[{1}:{2}] Processing ActivateNode for node {0}.", "id", "activityId", "sequenceNumber"),
                ActivateNodeReply(id, 56, "Reply_ActivateNode", Common::LogLevel::Info, "[{1}] ActivateNode for node {0} completed with error {2}.", "id", "activityId", "error"),

                RecoverPartitionsRequest(id, 57, "Request_RecoverPartitions", Common::LogLevel::Info, "[{0}] Processing RecoverPartitions.", "activityId"),
                RecoverPartitionsReply(id, 58, "Reply_RecoverPartitions", Common::LogLevel::Info, "[{0}] RecoverPartitions completed with error {1}.", "activityId", "error"),
                RecoverPartitionRequest(id, 59, "Request_RecoverPartition", Common::LogLevel::Info, "[{0}] Processing RecoverPartition for {1}.", "activityId", "partitionId"),
                RecoverPartitionReply(id, 60, "Reply_RecoverPartition", Common::LogLevel::Info, "[{0}] RecoverPartition completed with error {1}.", "activityId", "error"),
                RecoverServicePartitionsRequest(id, 61, "Request_RecoverServicePartitions", Common::LogLevel::Info, "[{0}] Processing RecoverServicePartitions for {1}.", "activityId", "serviceName"),
                RecoverServicePartitionsReply(id, 62, "Reply_RecoverServicePartitions", Common::LogLevel::Info, "[{0}] RecoverServicePartitions completed with error {1}.", "activityId", "error"),
                RecoverSystemPartitionsRequest(id, 63, "Request_RecoverSystemPartitions", Common::LogLevel::Info, "[{0}] Processing RecoverSystemPartitions.", "activityId"),
                RecoverSystemPartitionsReply(id, 64, "Reply_RecoverSystemPartitions", Common::LogLevel::Info, "[{0}] RecoverSystemPartitions completed with error {1}.", "activityId", "error"),

                CreateServiceRequest(id, 65, "Request_CreateService", Common::LogLevel::Info, "[{0}] Processing CreateService: Name={1}, Instance={2}", "activityId", "serviceName", "serviceInstance"),
                CreateServiceReply(id, 66, "Reply_CreateService", Common::LogLevel::Info, "[{0}] CreateService completed with error {1}", "activityId", "error"),
                UpdateServiceRequest(id, 67, "Request_UpdateService", Common::LogLevel::Info, "[{0}] Processing UpdateService: Name={1}, Instance={2}, UpdateVersion={3}", "activityId", "serviceName", "serviceInstance", "updateVersion"),
                UpdateSystemServiceRequest(id, 68, "Request_System_UpdateService", Common::LogLevel::Info, "[{0}] Processing UpdateService: Name={1}", "activityId", "serviceName"),
                UpdateServiceReply(id, 69, "Reply_UpdateService", Common::LogLevel::Info, "[{0}] UpdateService completed with error {1}", "activityId", "error"),
                DeleteServiceRequest(id, 70, "Request_DeleteService", Common::LogLevel::Info, "[{0}] Processing DeleteService: Name={1}, Instance={2}, IsForce={3}", "activityId", "serviceName", "serviceInstance", "isForce"),
                DeleteServiceReply(id, 71, "Reply_DeleteService", Common::LogLevel::Info, "[{0}] DeleteService completed with error {1}", "activityId", "error"),
                ServiceDescriptionRequest(id, 72, "Request_GetServiceDescription", Common::LogLevel::Info, "[{0}] Processing GetServiceDescription for {1}", "activityId", "serviceName"),
                ServiceDescriptionReply(id, 73, "Reply_GetServiceDescription", Common::LogLevel::Info, "[{0}] GetServiceDescription completed with {1}", "activityId", "error"),
                CreateApplicationRequest(id, 74, "Request_CreateApplication", Common::LogLevel::Info, "[{0}] Processing CreateApplication: Id={1}, Instance={2}", "activityId", "applicationId", "applicationInstance"),
                CreateApplicationReply(id, 75, "Reply_CreateApplication", Common::LogLevel::Info, "[{0}] CreateApplication completed with error {1}", "activityId", "error"),
                DeleteApplicationRequest(id, 76, "Request_DeleteApplication", Common::LogLevel::Info, "[{0}] Processing DeleteApplication: Id={1}, Instance={2}", "activityId", "applicationId", "applicationInstance"),
                DeleteApplicationReply(id, 77, "Reply_DeleteApplication", Common::LogLevel::Info, "[{0}] DeleteApplication completed with {1}", "activityId", "error"),
                ServiceTypeEnabledRequest(id, 78, "Request_ServiceTypeEnabled", Common::LogLevel::Info, "[{0}] Processing ServiceTypeEnabled: NodeInstance={1}, SequenceNumber={2}", "messageId", "nodeInstance", "sequenceNumber"),
                ServiceTypeEnabledReply(id, 79, "Reply_ServiceTypeEnabled", Common::LogLevel::Info, "[{0}] ServiceTypeEnabled completed with {1}", "activityId", "error"),
                ServiceTypeDisabledRequest(id, 80, "RequestServiceTypeDisabled", Common::LogLevel::Info, "[{0}] Processing ServiceTypeDisabled: NodeInstance={1}, SequenceNumber={2}", "messageId", "nodeInstance", "sequenceNumber"),
                ServiceTypeDisabledReply(id, 81, "Reply_ServiceTypeDisabled", Common::LogLevel::Info, "[{0}] ServiceTypeDisabled completed with {1}", "activityId", "error"),
            
                InvalidMessageDropped(id, 82, "MessageDrop", Common::LogLevel::Warning, "Dropping message with id = {1} and action = {2} because it is invalid. Error = {3:x}", "id", "msgid", "action", "error"),
                
                ResolutionRequest(id, 83, "Request_Resolution", Common::LogLevel::Info, "[{0}] Processing ServiceTableRequest: {1}", "activityId", "request"),
                ResolutionReply(id, 84, "Reply_Resolution", Common::LogLevel::Info, "[{0}] Sending versions  ({2} entries) [{1}]", "activityId", "count", "entries"),
                
                QueryRequest(id, 85, "Request_Query", Common::LogLevel::Info, "[{0}] Processing Query", "activityId"),
                QueryReply(id, 86, "Reply_Query", Common::LogLevel::Info, "[{0}] Query completed with error {1}.", "activityId", "error"),
                
                ServiceTypeNotificationRequest(id, 87, "Request_ServiceTypeNotification", Common::LogLevel::Info, "[{0}] Processing ServiceTypeNotification message from {1}:\r\n{2}", "messageId", "from", "body"),
                ServiceTypeNotificationReply(id, 88, "Reply_ServiceTypeNotification", Common::LogLevel::Info, "[{0}] Sending ServiceTypeNotificationReply message to {1}:\r\n{2}", "messageId", "from", "replyBody"),
                ServiceTypeNotificationFailure(id, 89, "Failure_ServiceTypeNotification", Common::LogLevel::Warning, "ServiceTypeNotification processing for {0} failed with {1}", "serviceType", "error"),
                UpdateApplicationRequest(id, 90, "Request_UpdateApplication", Common::LogLevel::Info, "[{0}] Processing UpdateApplication: Id={1}, Instance={2}, UpdateId={3}", "activityId", "applicationId", "applicationInstance", "updateId"),
                UpdateApplicationReply(id, 91, "Reply_UpdateApplication", Common::LogLevel::Info, "[{0}] UpdateApplication completed with {1}", "activityId", "error"),
                
                ReplicaDown(id, 92, "Request_ReplicaDown", Common::LogLevel::Info, "Processing ReplicaDown message from {1}:\r\n{2}", "id", "from", "body"),
                ReplicaDownReply(id, 93, "Reply_ReplicaDown", Common::LogLevel::Info, "Sending ReplicaDownReply message to {1}:\r\n{2}", "id", "to", "body"),
                ReplicaDownTimeout(id, 94, "Timeout_ReplicaDown", Common::LogLevel::Warning, "{2} replica(s) pending from {1}: {3}", "id", "from", "pendingCount", "failoverUnitId"),
                
                FTReplicaDownReceive(id, 95, "Receive_ReplicaDown", Common::LogLevel::Info, "Receiving {1}", "id", "replicaDescription"),
                FTReplicaDownProcess(id, 96, "Process_ReplicaDown", Common::LogLevel::Noise, "Processing ReplicaDown from node {1}", "id", "from"),
                SlowMessageProcess(id, 97, "SlowMessageProcess", Common::LogLevel::Info, "Processing of {0} message took {1} ms", "id", "duration"),

                PartitionNotification(id, 98, "Request_PartitionNotification", Common::LogLevel::Info, "[{0}] Processing PartitionNotification: NodeInstance={1}, SequenceNumber={2}", "messageId", "nodeInstance", "sequenceNumber"),
                PartitionNotificationReply(id, 99, "Reply_PartitionNotification", Common::LogLevel::Info, "[{0}] PartitionNotification completed with {1}", "activityId", "error")
                
                // Event id limit: 109
            {
            }

        };
    }
}
