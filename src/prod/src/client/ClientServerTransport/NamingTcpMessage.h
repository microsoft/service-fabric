// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class NamingTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        static const Transport::Actor::Enum GatewayActor;

        // Request actions
        static Common::GlobalWString PingRequestAction;
        static Common::GlobalWString PingReplyAction;
        static Common::GlobalWString DeactivateNodeAction;
        static Common::GlobalWString ActivateNodeAction;
        static Common::GlobalWString DeactivateNodesBatchRequestAction;
        static Common::GlobalWString RemoveNodeDeactivationsRequestAction;
        static Common::GlobalWString GetNodeDeactivationStatusRequestAction;
        static Common::GlobalWString NodeStateRemovedAction;
        static Common::GlobalWString RecoverPartitionsAction;
        static Common::GlobalWString RecoverPartitionAction;
        static Common::GlobalWString RecoverServicePartitionsAction;
        static Common::GlobalWString RecoverSystemPartitionsAction;
        static Common::GlobalWString ReportFaultAction;
        static Common::GlobalWString CreateServiceAction;
        static Common::GlobalWString UpdateServiceAction;
        static Common::GlobalWString DeleteServiceAction;
        static Common::GlobalWString GetServiceDescriptionAction;
        static Common::GlobalWString CreateNameAction;
        static Common::GlobalWString DeleteNameAction;
        static Common::GlobalWString NameExistsAction;
        static Common::GlobalWString EnumerateSubNamesAction;
        static Common::GlobalWString EnumeratePropertiesAction;
        static Common::GlobalWString ResolveNameOwnerAction;
        static Common::GlobalWString PropertyBatchAction;
        static Common::GlobalWString ResolveServiceAction;		
        static Common::GlobalWString ResolveSystemServiceAction;
        static Common::GlobalWString PrefixResolveServiceAction;
        static Common::GlobalWString ResolvePartitionAction;
        static Common::GlobalWString ServiceLocationChangeListenerAction;
        static Common::GlobalWString ClientOperationFailureAction;
        static Common::GlobalWString DeleteServiceReplyAction;
        static Common::GlobalWString StartNodeAction;
        static Common::GlobalWString SetAclAction;
        static Common::GlobalWString GetAclAction;
        static Common::GlobalWString RegisterServiceNotificationFilterRequestAction;
        static Common::GlobalWString RegisterServiceNotificationFilterReplyAction;
        static Common::GlobalWString UnregisterServiceNotificationFilterRequestAction;
        static Common::GlobalWString UnregisterServiceNotificationFilterReplyAction;
        static Common::GlobalWString ServiceNotificationRequestAction;
        static Common::GlobalWString ServiceNotificationReplyAction;
        static Common::GlobalWString NotificationClientConnectionRequestAction;
        static Common::GlobalWString NotificationClientConnectionReplyAction;
        static Common::GlobalWString NotificationClientSynchronizationRequestAction;
        static Common::GlobalWString NotificationClientSynchronizationReplyAction;
        static Common::GlobalWString CreateSystemServiceRequestAction;
        static Common::GlobalWString DeleteSystemServiceRequestAction;
        static Common::GlobalWString DeleteSystemServiceReplyAction;
        static Common::GlobalWString ResolveSystemServiceRequestAction;
        static Common::GlobalWString ResolveSystemServiceReplyAction;
        static Common::GlobalWString ResetPartitionLoadAction;
        static Common::GlobalWString ToggleVerboseServicePlacementHealthReportingAction;
        static Common::GlobalWString CreateNetworkAction;
        static Common::GlobalWString DeleteNetworkAction;

        NamingTcpMessage(std::wstring const & action)
            : Client::ClientServerRequestMessage(action, actor_)
        {
        }

        NamingTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
        }

        NamingTcpMessage(
            std::wstring const & action,
            Common::ActivityId const & activityId,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body), activityId)
        {
        }

        static Client::ClientServerRequestMessageUPtr GetGatewayPingRequest()
        { 
            return Common::make_unique<NamingTcpMessage>(PingRequestAction);
        }

        static Client::ClientServerRequestMessageUPtr GetGatewayPingReply(std::unique_ptr<PingReplyMessageBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(PingRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateService(
            std::unique_ptr<Naming::CreateServiceMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(CreateServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateService(
            std::unique_ptr<Naming::UpdateServiceRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(UpdateServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteService(
            std::unique_ptr<Naming::DeleteServiceMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(DeleteServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetServiceDescription(
            std::unique_ptr<NamingUriMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(GetServiceDescriptionAction, std::move(body));
        }
      
        static Client::ClientServerRequestMessageUPtr GetDeactivateNode(
            std::unique_ptr<Naming::DeactivateNodeMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(DeactivateNodeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetActivateNode(
            std::wstring const & nodeName)
        { 
            return Common::make_unique<NamingTcpMessage>(ActivateNodeAction, Common::make_unique<Naming::ActivateNodeMessageBody>(nodeName));
        }

        static Client::ClientServerRequestMessageUPtr GetDeactivateNodesBatch(
            std::unique_ptr<Reliability::DeactivateNodesRequestMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(DeactivateNodesBatchRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRemoveNodeDeactivations(
            std::unique_ptr<Reliability::RemoveNodeDeactivationRequestMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(RemoveNodeDeactivationsRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNodeDeactivationStatus(
            std::unique_ptr<Reliability::NodeDeactivationStatusRequestMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(GetNodeDeactivationStatusRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNodeStateRemoved(
            std::wstring const & body)
        { 
            return Common::make_unique<NamingTcpMessage>(NodeStateRemovedAction, Common::make_unique<Naming::NodeStateRemovedMessageBody>(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRecoverPartitions()
        { 
            return Common::make_unique<NamingTcpMessage>(RecoverPartitionsAction, Common::make_unique<Naming::RecoverPartitionsMessageBody>());
        }

        static Client::ClientServerRequestMessageUPtr GetRecoverPartition(Reliability::PartitionId const & partitionId)
        { 
            return Common::make_unique<NamingTcpMessage>(RecoverPartitionAction, Common::make_unique<Naming::RecoverPartitionMessageBody>(partitionId));
        }

        static Client::ClientServerRequestMessageUPtr GetRecoverServicePartitions(std::wstring const & serviceName)
        { 
            return Common::make_unique<NamingTcpMessage>(RecoverServicePartitionsAction, Common::make_unique<Naming::RecoverServicePartitionsMessageBody>(serviceName));
        }
        
        static Client::ClientServerRequestMessageUPtr GetRecoverSystemPartitions()
        { 
            return Common::make_unique<NamingTcpMessage>(RecoverSystemPartitionsAction, Common::make_unique<Naming::RecoverSystemPartitionsMessageBody>());
        }

        static Client::ClientServerRequestMessageUPtr GetReportFaultRequest(std::unique_ptr<Reliability::ReportFaultRequestMessageBody> && body) 
        { 
            return Common::make_unique<NamingTcpMessage>(ReportFaultAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResetPartitionLoad(Reliability::PartitionId const& partitionId)
        {
            return Common::make_unique<NamingTcpMessage>(ResetPartitionLoadAction, Common::make_unique<Naming::ResetPartitionLoadMessageBody>(partitionId));
        }

        static Client::ClientServerRequestMessageUPtr GetToggleVerboseServicePlacementHealthReporting(bool enabled)
        {
            return Common::make_unique<NamingTcpMessage>(ToggleVerboseServicePlacementHealthReportingAction, Common::make_unique<Naming::ToggleVerboseServicePlacementHealthReportingMessageBody>(enabled));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateNetwork(
            std::unique_ptr<Management::NetworkInventoryManager::CreateNetworkMessageBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(CreateNetworkAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteNetwork(
            std::unique_ptr<Management::NetworkInventoryManager::DeleteNetworkMessageBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(DeleteNetworkAction, std::move(body));
        }

        // Naming store messages

        static Client::ClientServerRequestMessageUPtr GetNameCreate(std::unique_ptr<NamingUriMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(CreateNameAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNameDelete(std::unique_ptr<NamingUriMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(DeleteNameAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNameExists(std::unique_ptr<NamingUriMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(NameExistsAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetEnumerateSubNames(std::unique_ptr<Naming::EnumerateSubNamesRequest> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(EnumerateSubNamesAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetPropertyBatch(std::unique_ptr<Naming::NamePropertyOperationBatch> && body)
        {
            return Common::make_unique<NamingTcpMessage>(PropertyBatchAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetEnumerateProperties(std::unique_ptr<Naming::EnumeratePropertiesRequest> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(EnumeratePropertiesAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolveNameOwner(std::unique_ptr<NamingUriMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(ResolveNameOwnerAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolveService(std::unique_ptr<Naming::ServiceResolutionMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(ResolveServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolveSystemService(Common::ActivityId const & activityId, std::unique_ptr<Naming::ServiceResolutionMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(ResolveSystemServiceAction, activityId, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetPrefixResolveService(std::unique_ptr<Naming::ServiceResolutionMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(PrefixResolveServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolvePartition(std::unique_ptr<Reliability::ConsistencyUnitId> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(ResolvePartitionAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetServiceLocationChangeListenerMessage(std::unique_ptr<Naming::ServiceLocationNotificationRequestBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(ServiceLocationChangeListenerAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartNode(std::unique_ptr<Naming::StartNodeRequestMessageBody> && body)
        { 
            return Common::make_unique<NamingTcpMessage>(StartNodeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetSetAcl(std::unique_ptr<Naming::SetAclRequest> && body)
        {
            return Common::make_unique<NamingTcpMessage>(SetAclAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetAcl(std::unique_ptr<Naming::GetAclRequest> && body)
        {
            return Common::make_unique<NamingTcpMessage>(GetAclAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRegisterServiceNotificationFilterRequest(std::unique_ptr<Naming::RegisterServiceNotificationFilterRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(RegisterServiceNotificationFilterRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRegisterServiceNotificationFilterReply()
        {
            return Common::make_unique<NamingTcpMessage>(RegisterServiceNotificationFilterReplyAction);
        }

        static Client::ClientServerRequestMessageUPtr GetUnregisterServiceNotificationFilterRequest(std::unique_ptr<Naming::UnregisterServiceNotificationFilterRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(UnregisterServiceNotificationFilterRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUnregisterServiceNotificationFilterReply()
        {
            return Common::make_unique<NamingTcpMessage>(UnregisterServiceNotificationFilterReplyAction);
        }

        static Client::ClientServerRequestMessageUPtr GetServiceNotificationRequest(std::unique_ptr<Naming::ServiceNotificationRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(ServiceNotificationRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetServiceNotificationReply(std::unique_ptr<Naming::ServiceNotificationReplyBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(ServiceNotificationReplyAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNotificationClientConnectionRequest(std::unique_ptr<Naming::NotificationClientConnectionRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(NotificationClientConnectionRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNotificationClientConnectionReply(std::unique_ptr<Naming::NotificationClientConnectionReplyBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(NotificationClientConnectionReplyAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNotificationClientSynchronizationRequest(std::unique_ptr<Naming::NotificationClientSynchronizationRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(NotificationClientSynchronizationRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNotificationClientSynchronizationReply(std::unique_ptr<Naming::NotificationClientSynchronizationReplyBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(NotificationClientSynchronizationReplyAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateSystemServiceRequest(std::unique_ptr<Naming::CreateSystemServiceMessageBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(CreateSystemServiceRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteSystemServiceRequest(std::unique_ptr<Naming::DeleteSystemServiceMessageBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(DeleteSystemServiceRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolveSystemServiceRequest(std::unique_ptr<Naming::ResolveSystemServiceRequestBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(ResolveSystemServiceRequestAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetResolveSystemServiceReply(std::unique_ptr<Naming::ResolveSystemServiceReplyBody> && body)
        {
            return Common::make_unique<NamingTcpMessage>(ResolveSystemServiceReplyAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetClientOperationFailureReply(Common::ErrorCode &&error)
        {
            return Common::make_unique<NamingTcpMessage>(ClientOperationFailureAction, Common::make_unique<Naming::ClientOperationFailureMessageBody>(std::move(error)));
        }

    private:

        static const Transport::Actor::Enum actor_;
    };
}
