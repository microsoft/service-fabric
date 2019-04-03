// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{    
    class NamingMessage
    {
    public:
        // Naming Service sub-components
        static const Transport::Actor::Enum GatewayActor;
        static const Transport::Actor::Enum StoreServiceActor;
        static const Transport::Actor::Enum DefaultActor;

        // Service Administration protocol messages
        static Common::GlobalWString DeactivateNodeReplyAction;
        static Common::GlobalWString ActivateNodeReplyAction;
        static Common::GlobalWString DeactivateNodesBatchReplyAction;
        static Common::GlobalWString RemoveNodeDeactivationsReplyAction;
        static Common::GlobalWString GetNodeDeactivationStatusReplyAction;
        static Common::GlobalWString NodeStateRemovedReplyAction;
        static Common::GlobalWString RecoverPartitionsReplyAction;
        static Common::GlobalWString RecoverPartitionReplyAction;
        static Common::GlobalWString RecoverServicePartitionsReplyAction;
        static Common::GlobalWString RecoverSystemPartitionsReplyAction;
        static Common::GlobalWString ResetPartitionLoadReplyAction;
        static Common::GlobalWString ToggleVerboseServicePlacementHealthReportingReplyAction;
        static Common::GlobalWString CreateNetworkReplyAction;
        static Common::GlobalWString DeleteNetworkReplyAction;

        // Client protocol messages
        static Common::GlobalWString ClientOperationFailureAction;

        // Entree Service protocol messages
        static Common::GlobalWString ServiceOperationReplyAction;
        static Common::GlobalWString DeleteServiceReplyAction;
        static Common::GlobalWString DeleteSystemServiceReplyAction;
        static Common::GlobalWString GetServiceDescriptionReplyAction;
        static Common::GlobalWString NameOperationReplyAction;
        static Common::GlobalWString NamingServiceCommunicationFailureReplyAction;
        static Common::GlobalWString NamingStaleRequestFailureReplyAction;
        static Common::GlobalWString ResolveServiceReplyAction;
        static Common::GlobalWString StartNodeReplyAction;

        static Common::GlobalWString ResolvePartitionReplyAction;
        static Common::GlobalWString ServiceLocationChangeReplyAction;

        // Store Service protocol messages
        static Common::GlobalWString InnerCreateServiceAction;
        static Common::GlobalWString InnerUpdateServiceAction;
        static Common::GlobalWString InnerCreateServiceReplyAction;
        static Common::GlobalWString InnerUpdateServiceReplyAction;

        static Common::GlobalWString InnerDeleteServiceAction;
        static Common::GlobalWString InnerDeleteServiceReplyAction;

        static Common::GlobalWString InnerCreateNameAction;
        static Common::GlobalWString InnerCreateNameReplyAction;

        static Common::GlobalWString InnerDeleteNameAction;
        static Common::GlobalWString InnerDeleteNameReplyAction;

        static Common::GlobalWString PrefixResolveAction;
        static Common::GlobalWString PrefixResolveReplyAction;

        static Common::GlobalWString ServiceExistsAction;
        static Common::GlobalWString ServiceExistsReplyAction;

        // Gateway Forwarding
        static Common::GlobalWString ForwardMessageAction;
        static Common::GlobalWString ForwardToFileStoreMessageAction;
        static Common::GlobalWString ForwardToTvsMessageAction;

        // EntreeService to EntreeServiceProxy
        static Common::GlobalWString DisconnectClientAction;
        static Common::GlobalWString IpcFailureReplyAction;
        static Common::GlobalWString OperationSuccessAction;
        static Common::GlobalWString InitialHandshakeAction;

        //
        // Functions for building messages to message between two Naming components
        //

        // Service Administration Client / Store Service <-> Entree Service
        static Transport::MessageUPtr GetServiceOperationReply() { return CreateMessage(ServiceOperationReplyAction); }
        static Transport::MessageUPtr GetDeleteServiceReply(VersionedReplyBody const & body) { return CreateMessage(DeleteServiceReplyAction, body); }
        static Transport::MessageUPtr GetDeleteSystemServiceReply() { return CreateMessage(DeleteSystemServiceReplyAction); }
        static Transport::MessageUPtr GetServiceDescriptionReply(PartitionedServiceDescriptor const & body) { return CreateMessage(GetServiceDescriptionReplyAction, body); }
        
        static Transport::MessageUPtr GetDeactivateNodeReply() { return CreateMessage(DeactivateNodeReplyAction); }
        static Transport::MessageUPtr GetActivateNodeReply() { return CreateMessage(ActivateNodeReplyAction); }
        static Transport::MessageUPtr GetDeactivateNodesBatchReply() { return CreateMessage(DeactivateNodesBatchReplyAction); }
        static Transport::MessageUPtr GetRemoveNodeDeactivationsReply() { return CreateMessage(RemoveNodeDeactivationsReplyAction); }
        static Transport::MessageUPtr GetNodeDeactivationStatusReply(Reliability::NodeDeactivationStatusReplyMessageBody const& body) { return CreateMessage(GetNodeDeactivationStatusReplyAction, body); }
        static Transport::MessageUPtr GetNodeStateRemovedReply() { return CreateMessage(NodeStateRemovedReplyAction); }
        static Transport::MessageUPtr GetRecoverPartitionsReply() { return CreateMessage(RecoverPartitionsReplyAction); }
        static Transport::MessageUPtr GetRecoverPartitionReply() { return CreateMessage(RecoverPartitionReplyAction); }
        static Transport::MessageUPtr GetRecoverServicePartitionsReply() { return CreateMessage(RecoverServicePartitionsReplyAction); }
        static Transport::MessageUPtr GetRecoverSystemPartitionsReply() { return CreateMessage(RecoverSystemPartitionsReplyAction); }
        static Transport::MessageUPtr GetResetPartitionLoadReply() { return CreateMessage(ResetPartitionLoadReplyAction); }
        static Transport::MessageUPtr GetToggleVerboseServicePlacementHealthReportingReply() { return CreateMessage(ToggleVerboseServicePlacementHealthReportingReplyAction); }
        static Transport::MessageUPtr GetCreateNetworkReply() { return CreateMessage(CreateNetworkReplyAction); }
        static Transport::MessageUPtr GetDeleteNetworkReply() { return CreateMessage(DeleteNetworkReplyAction); }

        // Naming Client <-> Entree Service
        static Transport::MessageUPtr GetNameOperationReply() { return CreateMessage(NameOperationReplyAction); }
        static Transport::MessageUPtr GetNameExistsReply(NameExistsReplyMessageBody const & body) { return CreateMessage(NameOperationReplyAction, body); }
        static Transport::MessageUPtr GetEnumerateSubNamesReply(EnumerateSubNamesResult const & body) { return CreateMessage(NameOperationReplyAction, body); }
        static Transport::MessageUPtr GetPropertyBatchReply(NamePropertyOperationBatchResult const & body) { return CreateMessage(NameOperationReplyAction, body); }
        static Transport::MessageUPtr GetEnumeratePropertiesReply(EnumeratePropertiesResult const & body) { return CreateMessage(NameOperationReplyAction, body); }
        static Transport::MessageUPtr GetResolveServiceReply(ResolvedServicePartition const & body) { return CreateMessage(ClientServerTransport::NamingTcpMessage::ResolveServiceAction, body); }
        static Transport::MessageUPtr GetResolveSystemServiceReply(ResolvedServicePartition const & body) { return CreateMessage(ClientServerTransport::NamingTcpMessage::ResolveSystemServiceAction, body); }
        static Transport::MessageUPtr GetResolvePartitionReply(Reliability::ServiceTableEntry const & body) { return CreateMessage(ResolvePartitionReplyAction, body); }
        static Transport::MessageUPtr GetServiceLocationChangeListenerReply(ServiceLocationNotificationReplyBody const & body) { return CreateMessage(ServiceLocationChangeReplyAction, body); }

        // Entree Service <-> Entree Service Proxy
        static Transport::MessageUPtr GetInitialHandshake() { return CreateMessage(InitialHandshakeAction, Transport::Actor::Enum::EntreeServiceTransport); }
        static Transport::MessageUPtr GetInitialHandshakeReply(EntreeServiceIpcInitResponseMessageBody const &body) { return CreateMessage(InitialHandshakeAction, body, Transport::Actor::EntreeServiceProxy); }
        static Transport::MessageUPtr GetClientOperationFailure(Transport::MessageId requestId, Common::ErrorCode && error)
        {
            Transport::MessageUPtr message = CreateMessage(ClientOperationFailureAction, ClientOperationFailureMessageBody(std::move(error)));
            message->Headers.Add(Transport::RelatesToHeader(requestId));
            return std::move(message);
        }

        static Transport::MessageUPtr GetIpcOperationFailure(Common::ErrorCode const &error, Transport::Actor::Enum actor = DefaultActor)
        {
            Transport::MessageUPtr message = CreateMessage(IpcFailureReplyAction, SystemServices::IpcFailureBody(error), actor);
            return std::move(message);
        }

        static Transport::MessageUPtr GetDisconnectClient(Transport::Actor::Enum actor = DefaultActor) { return CreateMessage(DisconnectClientAction, actor); }

        static Transport::MessageUPtr GetOperationSuccess(Transport::Actor::Enum actor = DefaultActor) { return CreateMessage(OperationSuccessAction, actor); }

        static Transport::MessageUPtr GetStartNodeReply() { return CreateMessage(StartNodeReplyAction);}

        // Entree Service <-> Store Service
        static Transport::MessageUPtr GetPeerCreateService(CreateServiceMessageBody const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::CreateServiceAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerUpdateService(UpdateServiceRequestBody const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::UpdateServiceAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerDeleteService(DeleteServiceMessageBody const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::DeleteServiceAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerGetServiceDescription(ServiceCheckRequestBody const & body) 
        { 
            return CreateMessage(ServiceExistsAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerNameCreate(Common::NamingUri const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::CreateNameAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerNameDelete(Common::NamingUri const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::DeleteNameAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerNameOperationReply() { return CreateMessage(NameOperationReplyAction, StoreServiceActor); }
        static Transport::MessageUPtr GetPeerNameExists(Common::NamingUri const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::NameExistsAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerNameExistsReply(NameExistsReplyMessageBody const & body) { return CreateMessage(NameOperationReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetPeerEnumerateSubNames(EnumerateSubNamesRequest const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::EnumerateSubNamesAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerEnumerateSubNamesReply(EnumerateSubNamesResult const & body) { return CreateMessage(NameOperationReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetPeerPropertyBatch(NamePropertyOperationBatch const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::PropertyBatchAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerPropertyBatchReply(NamePropertyOperationBatchResult const & body) { return CreateMessage(NameOperationReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetPeerEnumerateProperties(EnumeratePropertiesRequest const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::EnumeratePropertiesAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPeerEnumeratePropertiesReply(EnumeratePropertiesResult const & body) { return CreateMessage(NameOperationReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetPeerNamingServiceCommunicationFailureReply(CacheModeHeader const & header) 
        { 
            Transport::MessageUPtr message = CreateMessage(NamingServiceCommunicationFailureReplyAction, StoreServiceActor);
            message->Headers.Replace(header);
            return std::move(message);
        }
        static Transport::MessageUPtr GetStaleRequestFailureReply(StaleRequestFailureReplyBody const & body)
        {
            return CreateMessage(NamingStaleRequestFailureReplyAction, body, StoreServiceActor);
        }

        static Transport::MessageUPtr GetPrefixResolveRequest(ServiceCheckRequestBody const & body) 
        { 
            return CreateMessage(PrefixResolveAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetPrefixResolveReply(ServiceCheckReplyBody const & body) 
        { 
            return CreateMessage(PrefixResolveReplyAction, body, StoreServiceActor); 
        }

        // Store Service <-> Store Service
        static Transport::MessageUPtr GetInnerCreateService(CreateServiceMessageBody const & body) { return CreateMessage(InnerCreateServiceAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerUpdateService(UpdateServiceRequestBody const & body) { return CreateMessage(InnerUpdateServiceAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerDeleteService(DeleteServiceMessageBody const & body) { return CreateMessage(InnerDeleteServiceAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerNameCreate(Common::NamingUri const & body) { return CreateMessage(InnerCreateNameAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerNameDelete(Common::NamingUri const & body) { return CreateMessage(InnerDeleteNameAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetRepairNameCheck(Common::NamingUri const & body) 
        { 
            return CreateMessage(ClientServerTransport::NamingTcpMessage::NameExistsAction, body, StoreServiceActor); 
        }

        static Transport::MessageUPtr GetServiceCheck(ServiceCheckRequestBody const & body) { return CreateMessage(ServiceExistsAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetServiceCheckReply(ServiceCheckReplyBody const & body) { return CreateMessage(ServiceExistsReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerCreateServiceOperationReply() { return CreateMessage(InnerCreateServiceReplyAction, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerUpdateServiceOperationReply(UpdateServiceReplyBody const & body) { return CreateMessage(InnerUpdateServiceReplyAction, body, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerDeleteServiceOperationReply() { return CreateMessage(InnerDeleteServiceReplyAction, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerCreateNameOperationReply() { return CreateMessage(InnerCreateNameReplyAction, StoreServiceActor); }
        static Transport::MessageUPtr GetInnerDeleteNameOperationReply() { return CreateMessage(InnerDeleteNameReplyAction, StoreServiceActor); }
        
        static bool IsNORequest(std::wstring const & action);

        static Common::ActivityId GetActivityId(Transport::Message & message);

        template <class THeader>
        static bool TryGetNamingHeader(Transport::Message & message, __out THeader & header)
        {
            return message.Headers.TryReadFirst(header);
        }

    private:
        static Transport::MessageUPtr CreateMessage(std::wstring const & action, Transport::Actor::Enum actor = DefaultActor);

        template <class TBody> 
        static Transport::MessageUPtr CreateMessage(std::wstring const & action, TBody const & body, Transport::Actor::Enum actor = DefaultActor)
        {
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
            NamingMessage::SetHeaders(*message, action, actor);
            return std::move(message);
        }

        template <class THeader, class TBody> 
        static Transport::MessageUPtr CreateMessage(std::wstring const & action, THeader const & header, TBody const & body, Transport::Actor::Enum actor = DefaultActor)
        {
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
            NamingMessage::SetHeaders(*message, action, actor);
            message->Headers.Add(header);

            return std::move(message);
        }

        static void SetHeaders(Transport::Message &, std::wstring const &, Transport::Actor::Enum);
    };
}
