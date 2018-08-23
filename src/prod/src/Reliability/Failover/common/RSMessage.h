// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class RSMessage
    {
    public:
        RSMessage(
            Transport::Actor::Enum actor,
            std::wstring const & action,
            bool isDefaultHighPriority)
            : actor_(actor),
              actionHeader_(action),
              idempotent_(true),
              isDefaultHighPriority_(isDefaultHighPriority)
        {
        }

        static RSMessage const & GetNodeUp() { return NodeUp; }
        static RSMessage const & GetNodeUpAck() { return NodeUpAck; }
        static RSMessage const & GetChangeNotification() { return ChangeNotification; }
        static RSMessage const & GetChangeNotificationAck() { return ChangeNotificationAck; }
        static RSMessage const & GetChangeNotificationChallenge() { return ChangeNotificationChallenge; }
        static RSMessage const & GetCreateService() { return CreateService; }
        static RSMessage const & GetCreateServiceReply() { return CreateServiceReply; }
        static RSMessage const & GetUpdateService() { return UpdateService; }
        static RSMessage const & GetUpdateSystemService() { return UpdateSystemService; }
        static RSMessage const & GetUpdateServiceReply() { return UpdateServiceReply; }
        static RSMessage const & GetDeleteApplicationRequest() { return DeleteApplicationRequest; }
        static RSMessage const & GetDeleteApplicationReply() { return DeleteApplicationReply; }
        static RSMessage const & GetDeleteService() { return DeleteService; }
        static RSMessage const & GetDeleteServiceReply() { return DeleteServiceReply; }
        static RSMessage const & GetServiceTypeEnabled() { return ServiceTypeEnabled; }
        static RSMessage const & GetServiceTypeEnabledReply() { return ServiceTypeEnabledReply; }
        static RSMessage const & GetServiceTypeDisabled() { return ServiceTypeDisabled; }
        static RSMessage const & GetServiceTypeDisabledReply() { return ServiceTypeDisabledReply; }
        static RSMessage const & GetServiceTableRequest() { return ServiceTableRequest; }
        static RSMessage const & GetServiceTableUpdate() { return ServiceTableUpdate; }
        static RSMessage const & GetAddInstance() { return AddInstance; }
        static RSMessage const & GetAddInstanceReply() { return AddInstanceReply; }
        static RSMessage const & GetRemoveInstance() { return RemoveInstance; }
        static RSMessage const & GetRemoveInstanceReply() { return RemoveInstanceReply; }
        static RSMessage const & GetAddPrimary() { return AddPrimary; }
        static RSMessage const & GetAddPrimaryReply() { return AddPrimaryReply; }
        static RSMessage const & GetDoReconfiguration() { return DoReconfiguration; }
        static RSMessage const & GetDoReconfigurationReply() { return DoReconfigurationReply; }
        static RSMessage const & GetAddReplica() { return AddReplica; }
        static RSMessage const & GetAddReplicaReply() { return AddReplicaReply; }
        static RSMessage const & GetRemoveReplica() { return RemoveReplica; }
        static RSMessage const & GetRemoveReplicaReply() { return RemoveReplicaReply; }
        static RSMessage const & GetDeleteReplica() { return DeleteReplica; }
        static RSMessage const & GetDeleteReplicaReply() { return DeleteReplicaReply; }
        static RSMessage const & GetChangeConfiguration() { return ChangeConfiguration; }
        static RSMessage const & GetChangeConfigurationReply() { return ChangeConfigurationReply; }
        static RSMessage const & GetDatalossReport() { return DatalossReport; }
        static RSMessage const & GetReplicaDropped() { return ReplicaDropped; }
        static RSMessage const & GetReplicaDroppedReply() { return ReplicaDroppedReply; }
        static RSMessage const & GetReplicaDown() { return ReplicaDown; }
        static RSMessage const & GetReplicaDownReply() { return ReplicaDownReply; }
        static RSMessage const & GetReplicaUp() { return ReplicaUp; }
        static RSMessage const & GetReplicaUpReply() { return ReplicaUpReply; }
        static RSMessage const & GetReplicaEndpointUpdated() { return ReplicaEndpointUpdated; }
        static RSMessage const & GetReplicaEndpointUpdatedReply() { return ReplicaEndpointUpdatedReply; }
        static RSMessage const & GetDeactivate() { return Deactivate; }
        static RSMessage const & GetDeactivateReply() { return DeactivateReply; }
        static RSMessage const & GetActivate() { return Activate; }
        static RSMessage const & GetActivateReply() { return ActivateReply; }
        static RSMessage const & GetCreateReplica() { return CreateReplica; }
        static RSMessage const & GetCreateReplicaReply() { return CreateReplicaReply; }
        static RSMessage const & GetGetLSN() { return GetLSN; }
        static RSMessage const & GetGetLSNReply() { return GetLSNReply; }
        static RSMessage const & GetReportLoad() { return ReportLoad; }

        static RSMessage const & GetDeactivateNode() { return DeactivateNode; }
        static RSMessage const & GetDeactivateNodeReply() { return DeactivateNodeReply; }

        static RSMessage const & GetActivateNode() { return ActivateNode; }
        static RSMessage const & GetActivateNodeReply() { return ActivateNodeReply; }

        static RSMessage const & GetDeactivateNodesRequest() { return DeactivateNodesRequest; }
        static RSMessage const & GetDeactivateNodesReply() { return DeactivateNodesReply; }

        static RSMessage const & GetNodeDeactivationStatusRequest() { return NodeDeactivationStatusRequest; }
        static RSMessage const & GetNodeDeactivationStatusReply() { return NodeDeactivationStatusReply; }

        static RSMessage const & GetRemoveNodeDeactivationRequest() { return RemoveNodeDeactivationRequest; }
        static RSMessage const & GetRemoveNodeDeactivationReply() { return RemoveNodeDeactivationReply; }

        static RSMessage const & GetNodeStateRemoved() { return NodeStateRemoved; }
        static RSMessage const & GetNodeStateRemovedReply() { return NodeStateRemovedReply; }

        static RSMessage const & GetRecoverPartitions() { return RecoverPartitions; }
        static RSMessage const & GetRecoverPartitionsReply() { return RecoverPartitionsReply; }

        static RSMessage const & GetRecoverPartition() { return RecoverPartition; }
        static RSMessage const & GetRecoverPartitionReply() { return RecoverPartitionReply; }

        static RSMessage const & GetRecoverServicePartitions() { return RecoverServicePartitions; }
        static RSMessage const & GetRecoverServicePartitionsReply() { return RecoverServicePartitionsReply; }

        static RSMessage const & GetRecoverSystemPartitions() { return RecoverSystemPartitions; }
        static RSMessage const & GetRecoverSystemPartitionsReply() { return RecoverSystemPartitionsReply; }

        static RSMessage const & GetQueryRequest() { return QueryRequest; }
        static RSMessage const & GetQueryReply() { return QueryReply; }

        static RSMessage const & GetGenerationProposal() { return GenerationProposal; }
        static RSMessage const & GetGenerationProposalReply() { return GenerationProposalReply; }
        static RSMessage const & GetGenerationUpdate() { return GenerationUpdate; }
        static RSMessage const & GetGenerationUpdateReject() { return GenerationUpdateReject; }
        static RSMessage const & GetLFUMUpload() { return LFUMUpload; }
        static RSMessage const & GetLFUMUploadReply() { return LFUMUploadReply; }
        static RSMessage const & GetGFUMTransfer() { return GFUMTransfer; }

        static RSMessage const & GetApplicationUpgradeRequest() { return ApplicationUpgradeRequest; }
        static RSMessage const & GetApplicationUpgradeReply() { return ApplicationUpgradeReply; }

        static RSMessage const & GetFabricUpgradeRequest() { return FabricUpgradeRequest; }
        static RSMessage const & GetFabricUpgradeReply() { return FabricUpgradeReply; }

        static RSMessage const & GetNodeFabricUpgradeRequest() { return NodeFabricUpgradeRequest; }
        static RSMessage const & GetNodeFabricUpgradeReply() { return NodeFabricUpgradeReply; }

        static RSMessage const & GetNodeUpgradeRequest() { return NodeUpgradeRequest; }
        static RSMessage const & GetNodeUpgradeReply() { return NodeUpgradeReply; }

        static RSMessage const & GetCancelApplicationUpgradeRequest() { return CancelApplicationUpgradeRequest; }
        static RSMessage const & GetCancelApplicationUpgradeReply() { return CancelApplicationUpgradeReply; }

        static RSMessage const & GetCancelFabricUpgradeRequest() { return CancelFabricUpgradeRequest; }
        static RSMessage const & GetCancelFabricUpgradeReply() { return CancelFabricUpgradeReply; }

        static RSMessage const & GetNodeUpdateServiceRequest() { return NodeUpdateServiceRequest; }
        static RSMessage const & GetNodeUpdateServiceReply() { return NodeUpdateServiceReply; }

        static RSMessage const & GetNodeDeactivateRequest() { return NodeDeactivateRequest; }
        static RSMessage const & GetNodeDeactivateReply() { return NodeDeactivateReply; }

        static RSMessage const & GetNodeActivateRequest() { return NodeActivateRequest; }
        static RSMessage const & GetNodeActivateReply() { return NodeActivateReply; }        

        static RSMessage const & GetReportFaultRequest() { return ReportFaultRequest; }
        static RSMessage const & GetReportFaultReply() { return ReportFaultReply; }

        static RSMessage const & GetServiceDescriptionRequest() { return ServiceDescriptionRequest; }
        static RSMessage const & GetServiceDescriptionReply() { return ServiceDescriptionReply;  }

        static RSMessage const & GetResetPartitionLoad() { return ResetPartitionLoad; }
        static RSMessage const & GetResetPartitionLoadReply() { return ResetPartitionLoadReply; }

        static RSMessage const & GetToggleVerboseServicePlacementHealthReporting() { return ToggleVerboseServicePlacementHealthReporting; }
        static RSMessage const & GetToggleVerboseServicePlacementHealthReportingReply() { return ToggleVerboseServicePlacementHealthReportingReply; }

        static RSMessage const & GetServiceTypeNotification() { return ServiceTypeNotification; }
        static RSMessage const & GetServiceTypeNotificationReply() { return ServiceTypeNotificationReply; }
		
        static RSMessage const & GetCreateApplicationRequest() { return CreateApplicationRequest; }
        static RSMessage const & GetCreateApplicationReply() { return CreateApplicationReply; }

        static RSMessage const & GetUpdateApplicationRequest() { return UpdateApplicationRequest; }
        static RSMessage const & GetUpdateApplicationReply() { return UpdateApplicationReply; } 

        static RSMessage const & GetServiceBusy() { return ServiceBusy; }

        static RSMessage const & GetPartitionNotification() { return PartitionNotification; }
        static RSMessage const & GetPartitionNotificationReply() { return PartitionNotificationReply; }

        static RSMessage const & GetAvailableContainerImages() { return AvailableContainerImages; }

        // Creates a message without a body
        Transport::MessageUPtr CreateMessage(bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
            AddHeaders(*message, isHighPriority);

            return message;
        }

        // Creates a message with the specified body
        template <class T> 
        Transport::MessageUPtr CreateMessage(T const& t, bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>(t);
            AddHeaders(*message, isHighPriority);

            return message;
        }       

        // Creates a message with the specified body and adds partitionId to Message property if UnreliableTransport is enabled.
        template <class T>
        Transport::MessageUPtr CreateMessage(T const& t, Common::Guid const & partitionId, bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>(t);
            Transport::UnreliableTransport::AddPartitionIdToMessageProperty(*message, partitionId);
            AddHeaders(*message, isHighPriority);

            return message;
        }     

        template <class T>
        Transport::MessageUPtr CreateMessage(Transport::FabricActivityHeader const & activityHeader, T const & body, bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>(body);
            AddHeaders(*message, isHighPriority);
            AddActivityHeader(*message, activityHeader);
            return message;
        }

        template <class T>
        Transport::MessageUPtr CreateMessage(Common::ActivityId const & activityId, T const & body, bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>(body);
            AddHeaders(*message, isHighPriority);
            AddActivityHeader(*message, Transport::FabricActivityHeader(activityId));
            return message;
        }

        template <class T>
        Transport::MessageUPtr CreateMessage(Common::ActivityId const & activityId, bool isHighPriority = false) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
            AddHeaders(*message, isHighPriority);
            AddActivityHeader(*message, Transport::FabricActivityHeader(activityId));
            return message;
        }

        __declspec (property(get=get_Action)) const std::wstring & Action;
        std::wstring const& get_Action() const { return actionHeader_.Action; } ;        

        static void AddActivityHeader(Transport::Message & message, Transport::FabricActivityHeader const & activityHeader);
        static Common::ActivityId RSMessage::GetActivityId(Transport::Message & message);

        static Transport::Actor::Enum const FMActor;
        static Transport::Actor::Enum const FMMActor;
        static Transport::Actor::Enum const RSActor;
        static Transport::Actor::Enum const RaActor;
        static Transport::Actor::Enum const ServiceResolverActor;

    private:
        static Common::Global<RSMessage> CreateCommonToFMMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(FMActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateFMToCommonMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(RSActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateMessageToServiceResolver(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(ServiceResolverActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateFMToRAMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(RaActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateRAToFMMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(FMActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateRAToRAMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(RaActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateCommonToRAMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(RaActor, action, isDefaultHighPriority);
        }

        static Common::Global<RSMessage> CreateRAToCommonMessage(std::wstring const & action, bool isDefaultHighPriority = false)
        {
            return Common::make_global<RSMessage>(RSActor, action, isDefaultHighPriority);
        }

        void AddHeaders(Transport::Message & message, bool isHighPriority) const;

        static Common::Global<RSMessage> NodeUp;
        static Common::Global<RSMessage> NodeUpAck;
        static Common::Global<RSMessage> ChangeNotification;
        static Common::Global<RSMessage> ChangeNotificationAck;
        static Common::Global<RSMessage> ChangeNotificationChallenge;
        static Common::Global<RSMessage> CreateService;
        static Common::Global<RSMessage> CreateServiceReply;
        static Common::Global<RSMessage> UpdateService;
        static Common::Global<RSMessage> UpdateSystemService;
        static Common::Global<RSMessage> UpdateServiceReply;
        static Common::Global<RSMessage> DeleteApplicationRequest;
        static Common::Global<RSMessage> DeleteApplicationReply;
        static Common::Global<RSMessage> DeleteService;
        static Common::Global<RSMessage> DeleteServiceReply;
        static Common::Global<RSMessage> ServiceTypeEnabled;
        static Common::Global<RSMessage> ServiceTypeEnabledReply;
        static Common::Global<RSMessage> ServiceTypeDisabled;
        static Common::Global<RSMessage> ServiceTypeDisabledReply;
        static Common::Global<RSMessage> ServiceTableRequest;
        static Common::Global<RSMessage> ServiceTableUpdate;
        static Common::Global<RSMessage> AddInstance;
        static Common::Global<RSMessage> AddInstanceReply;
        static Common::Global<RSMessage> RemoveInstance;
        static Common::Global<RSMessage> RemoveInstanceReply;
        static Common::Global<RSMessage> AddPrimary;
        static Common::Global<RSMessage> AddPrimaryReply;
        static Common::Global<RSMessage> DoReconfiguration;
        static Common::Global<RSMessage> DoReconfigurationReply;
        static Common::Global<RSMessage> AddReplica;
        static Common::Global<RSMessage> AddReplicaReply;
        static Common::Global<RSMessage> RemoveReplica;
        static Common::Global<RSMessage> RemoveReplicaReply;
        static Common::Global<RSMessage> DropReplica;
        static Common::Global<RSMessage> DropReplicaReply;
        static Common::Global<RSMessage> DeleteReplica;
        static Common::Global<RSMessage> DeleteReplicaReply;
        static Common::Global<RSMessage> ChangeConfiguration;
        static Common::Global<RSMessage> ChangeConfigurationReply;
        static Common::Global<RSMessage> DatalossReport;
        static Common::Global<RSMessage> ReplicaDropped;
        static Common::Global<RSMessage> ReplicaDroppedReply;
        static Common::Global<RSMessage> ReplicaDown;
        static Common::Global<RSMessage> ReplicaDownReply;
        static Common::Global<RSMessage> ReplicaUp;
        static Common::Global<RSMessage> ReplicaUpReply;
        static Common::Global<RSMessage> ReplicaEndpointUpdated;
        static Common::Global<RSMessage> ReplicaEndpointUpdatedReply;
        static Common::Global<RSMessage> Deactivate;
        static Common::Global<RSMessage> DeactivateReply;
        static Common::Global<RSMessage> Activate;
        static Common::Global<RSMessage> ActivateReply;        
        static Common::Global<RSMessage> CreateReplica;
        static Common::Global<RSMessage> CreateReplicaReply;
        static Common::Global<RSMessage> GetLSN;
        static Common::Global<RSMessage> GetLSNReply;
        static Common::Global<RSMessage> ReportLoad;
        static Common::Global<RSMessage> DeactivateNode;
        static Common::Global<RSMessage> DeactivateNodeReply;
        static Common::Global<RSMessage> ActivateNode;
        static Common::Global<RSMessage> ActivateNodeReply;
        static Common::Global<RSMessage> DeactivateNodesRequest;
        static Common::Global<RSMessage> DeactivateNodesReply;
        static Common::Global<RSMessage> NodeDeactivationStatusRequest;
        static Common::Global<RSMessage> NodeDeactivationStatusReply;
        static Common::Global<RSMessage> RemoveNodeDeactivationRequest;
        static Common::Global<RSMessage> RemoveNodeDeactivationReply;
        static Common::Global<RSMessage> NodeStateRemoved;
        static Common::Global<RSMessage> NodeStateRemovedReply;
        static Common::Global<RSMessage> RecoverPartitions;
        static Common::Global<RSMessage> RecoverPartitionsReply;
        static Common::Global<RSMessage> RecoverPartition;
        static Common::Global<RSMessage> RecoverPartitionReply;
        static Common::Global<RSMessage> RecoverServicePartitions;
        static Common::Global<RSMessage> RecoverServicePartitionsReply;
        static Common::Global<RSMessage> RecoverSystemPartitions;
        static Common::Global<RSMessage> RecoverSystemPartitionsReply;
        static Common::Global<RSMessage> QueryRequest;
        static Common::Global<RSMessage> QueryReply;
        static Common::Global<RSMessage> GenerationProposal;
        static Common::Global<RSMessage> GenerationProposalReply;
        static Common::Global<RSMessage> GenerationUpdate;
        static Common::Global<RSMessage> GenerationUpdateReject;
        static Common::Global<RSMessage> LFUMUpload;
        static Common::Global<RSMessage> LFUMUploadReply;
        static Common::Global<RSMessage> GFUMTransfer;
        static Common::Global<RSMessage> ApplicationUpgradeRequest;
        static Common::Global<RSMessage> ApplicationUpgradeReply;
        static Common::Global<RSMessage> FabricUpgradeRequest;
        static Common::Global<RSMessage> FabricUpgradeReply;
        static Common::Global<RSMessage> NodeFabricUpgradeRequest;
        static Common::Global<RSMessage> NodeFabricUpgradeReply;
        static Common::Global<RSMessage> NodeUpgradeRequest;
        static Common::Global<RSMessage> NodeUpgradeReply;
        static Common::Global<RSMessage> CancelApplicationUpgradeRequest;
        static Common::Global<RSMessage> CancelApplicationUpgradeReply;
        static Common::Global<RSMessage> CancelFabricUpgradeRequest;
        static Common::Global<RSMessage> CancelFabricUpgradeReply;
        static Common::Global<RSMessage> NodeUpdateServiceRequest;
        static Common::Global<RSMessage> NodeUpdateServiceReply;

        static Common::Global<RSMessage> NodeDeactivateRequest;
        static Common::Global<RSMessage> NodeDeactivateReply;

        static Common::Global<RSMessage> NodeActivateRequest;
        static Common::Global<RSMessage> NodeActivateReply;        

        static Common::Global<RSMessage> ReportFaultRequest;
        static Common::Global<RSMessage> ReportFaultReply;

        static Common::Global<RSMessage> ServiceDescriptionRequest;
        static Common::Global<RSMessage> ServiceDescriptionReply;

        static Common::Global<RSMessage> ResetPartitionLoad;
        static Common::Global<RSMessage> ResetPartitionLoadReply;

        static Common::Global<RSMessage> ToggleVerboseServicePlacementHealthReporting;
        static Common::Global<RSMessage> ToggleVerboseServicePlacementHealthReportingReply;

        static Common::Global<RSMessage> ServiceTypeNotification;
        static Common::Global<RSMessage> ServiceTypeNotificationReply;
		
        static Common::Global<RSMessage> CreateApplicationRequest;
        static Common::Global<RSMessage> CreateApplicationReply;

        static Common::Global<RSMessage> UpdateApplicationRequest;
        static Common::Global<RSMessage> UpdateApplicationReply; 

        static Common::Global<RSMessage> ServiceBusy;

        static Common::Global<RSMessage> PartitionNotification;
        static Common::Global<RSMessage> PartitionNotificationReply;

        static Common::Global<RSMessage> AvailableContainerImages;

        Transport::Actor::Enum actor_;
        Transport::ActionHeader actionHeader_;
        bool idempotent_;
        bool isDefaultHighPriority_;
    };
}
