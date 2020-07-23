// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace std;

namespace Naming
{
    const Transport::Actor::Enum NamingMessage::GatewayActor(Actor::NamingGateway);
    const Transport::Actor::Enum NamingMessage::StoreServiceActor(Actor::NamingStoreService);
    const Transport::Actor::Enum NamingMessage::DefaultActor(Actor::NamingGateway);

    Common::GlobalWString NamingMessage::DeactivateNodeReplyAction = Common::make_global<std::wstring>(L"DeactivateNodeReply");
    Common::GlobalWString NamingMessage::ActivateNodeReplyAction = Common::make_global<std::wstring>(L"ActivateNodeReply");
    Common::GlobalWString NamingMessage::NodeStateRemovedReplyAction = Common::make_global<std::wstring>(L"NodeStateRemovedReply");
    Common::GlobalWString NamingMessage::DeactivateNodesBatchReplyAction = Common::make_global<std::wstring>(L"DeactivateNodesBatchReply");
    Common::GlobalWString NamingMessage::RemoveNodeDeactivationsReplyAction = Common::make_global<std::wstring>(L"RemoveNodeDeactivationsReply");
    Common::GlobalWString NamingMessage::GetNodeDeactivationStatusReplyAction = Common::make_global<std::wstring>(L"GetNodeDeactivationStatusReply");
    Common::GlobalWString NamingMessage::RecoverPartitionsReplyAction = Common::make_global<std::wstring>(L"RecoverPartitionsReply");
    Common::GlobalWString NamingMessage::RecoverPartitionReplyAction = Common::make_global<std::wstring>(L"RecoverPartitionReply");
    Common::GlobalWString NamingMessage::RecoverServicePartitionsReplyAction = Common::make_global<std::wstring>(L"RecoverServicePartitionsReply");
    Common::GlobalWString NamingMessage::RecoverSystemPartitionsReplyAction = Common::make_global<std::wstring>(L"RecoverSystemPartitionsReply");
    Common::GlobalWString NamingMessage::ResetPartitionLoadReplyAction = Common::make_global<std::wstring>(L"ResetPartitionLoadReply");
    Common::GlobalWString NamingMessage::ToggleVerboseServicePlacementHealthReportingReplyAction = Common::make_global<std::wstring>(L"ToggleVerboseServicePlacementHealthReportingReply");
    Common::GlobalWString NamingMessage::CreateNetworkReplyAction = Common::make_global<std::wstring>(L"CreateNetworkReply");
    Common::GlobalWString NamingMessage::DeleteNetworkReplyAction = Common::make_global<std::wstring>(L"DeleteNetworkReply");

    Common::GlobalWString NamingMessage::ClientOperationFailureAction = Common::make_global<std::wstring>(L"ClientOperationFailure");

    Common::GlobalWString NamingMessage::ServiceOperationReplyAction = Common::make_global<std::wstring>(L"ServiceOperationReply");
    Common::GlobalWString NamingMessage::DeleteServiceReplyAction = Common::make_global<std::wstring>(L"DeleteServiceReply");
    Common::GlobalWString NamingMessage::DeleteSystemServiceReplyAction = Common::make_global<std::wstring>(L"DeleteSystemServiceReply");
    Common::GlobalWString NamingMessage::GetServiceDescriptionReplyAction = Common::make_global<std::wstring>(L"GetServiceDescriptionReply");
    Common::GlobalWString NamingMessage::NameOperationReplyAction = Common::make_global<std::wstring>(L"NameOperationReply");    
    Common::GlobalWString NamingMessage::NamingServiceCommunicationFailureReplyAction = Common::make_global<std::wstring>(L"NamingServiceCommunicationFailureReply");
    Common::GlobalWString NamingMessage::NamingStaleRequestFailureReplyAction = Common::make_global<std::wstring>(L"NamingStaleRequestFailureReply");
    Common::GlobalWString NamingMessage::ResolveServiceReplyAction = Common::make_global<std::wstring>(L"ResolveServiceReply");
    Common::GlobalWString NamingMessage::ResolvePartitionReplyAction = Common::make_global<std::wstring>(L"ResolvePartitionReply");
    Common::GlobalWString NamingMessage::ServiceLocationChangeReplyAction = Common::make_global<std::wstring>(L"ServiceLocationChangeReply");
    Common::GlobalWString NamingMessage::InnerCreateServiceAction = Common::make_global<std::wstring>(L"InnerCreateServiceRequest");
    Common::GlobalWString NamingMessage::InnerUpdateServiceAction = Common::make_global<std::wstring>(L"InnerUpdateServiceRequest");
    Common::GlobalWString NamingMessage::InnerCreateServiceReplyAction = Common::make_global<std::wstring>(L"InnerCreateServiceReply");
    Common::GlobalWString NamingMessage::InnerUpdateServiceReplyAction = Common::make_global<std::wstring>(L"InnerUpdateServiceReply");

    Common::GlobalWString NamingMessage::InnerDeleteServiceAction = Common::make_global<std::wstring>(L"InnerDeleteServiceRequest");
    Common::GlobalWString NamingMessage::InnerDeleteServiceReplyAction = Common::make_global<std::wstring>(L"InnerDeleteServiceReply");

    Common::GlobalWString NamingMessage::InnerCreateNameAction = Common::make_global<std::wstring>(L"InnerCreateNameRequest");
    Common::GlobalWString NamingMessage::InnerCreateNameReplyAction = Common::make_global<std::wstring>(L"InnerCreateNameReply");

    Common::GlobalWString NamingMessage::InnerDeleteNameAction = Common::make_global<std::wstring>(L"InnerDeleteNameRequest");
    Common::GlobalWString NamingMessage::InnerDeleteNameReplyAction = Common::make_global<std::wstring>(L"InnerDeleteNameReply");

    Common::GlobalWString NamingMessage::PrefixResolveAction = Common::make_global<std::wstring>(L"PrefixResolveRequest");
    Common::GlobalWString NamingMessage::PrefixResolveReplyAction = Common::make_global<std::wstring>(L"PrefixResolveReply");

    Common::GlobalWString NamingMessage::ServiceExistsAction = Common::make_global<std::wstring>(L"ServiceExistsRequest");
    Common::GlobalWString NamingMessage::ServiceExistsReplyAction = Common::make_global<std::wstring>(L"ServiceExistsReply");

    Common::GlobalWString NamingMessage::ForwardMessageAction = Common::make_global<std::wstring>(L"ForwardMessage");       
    Common::GlobalWString NamingMessage::ForwardToFileStoreMessageAction = Common::make_global<std::wstring>(L"ForwardToFileStoreMessage");       
    Common::GlobalWString NamingMessage::ForwardToTvsMessageAction = Common::make_global<std::wstring>(L"ForwardToTvsMessage");

    Common::GlobalWString NamingMessage::StartNodeReplyAction = Common::make_global<std::wstring>(L"StartNodeReplyAction");    

    Common::GlobalWString NamingMessage::IpcFailureReplyAction = Common::make_global<std::wstring>(L"IpcFailureReplyAction");
    Common::GlobalWString NamingMessage::DisconnectClientAction = Common::make_global<std::wstring>(L"DisconnectClientAction");
    Common::GlobalWString NamingMessage::OperationSuccessAction = Common::make_global<std::wstring>(L"OperationSuccessAction");
    Common::GlobalWString NamingMessage::InitialHandshakeAction = Common::make_global<std::wstring>(L"InitialHandshakeAction");

    MessageUPtr NamingMessage::CreateMessage(std::wstring const & action, Transport::Actor::Enum actor)
    {
        MessageUPtr message = Common::make_unique<Message>();
        NamingMessage::SetHeaders(*message, action, actor);
        return std::move(message);
    }

    void NamingMessage::SetHeaders(Message & message, std::wstring const & action, Transport::Actor::Enum actor)
    {
        message.Headers.Add(ActionHeader(action));
        message.Headers.Add(ActorHeader(actor));
        message.Headers.Add(FabricActivityHeader(Guid::NewGuid()));
    }

    ActivityId NamingMessage::GetActivityId(Message & message)
    {
        return FabricActivityHeader::FromMessage(message).ActivityId;
    }

    bool NamingMessage::IsNORequest(std::wstring const & action)
    {
        return action == *NamingMessage::InnerCreateNameAction ||
               action == *NamingMessage::InnerCreateServiceAction ||
               action == *NamingMessage::InnerDeleteNameAction ||
               action == *NamingMessage::InnerDeleteServiceAction ||
               action == *NamingMessage::InnerUpdateServiceAction;
    }
}
