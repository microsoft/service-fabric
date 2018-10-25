// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace SystemServices;

StringLiteral const TraceComponent("ServiceRoutingAgentMessage");

GlobalWString ServiceRoutingAgentMessage::IpcFailureAction = make_global<wstring>(L"IpcFailureAction");

GlobalWString ServiceRoutingAgentMessage::ServiceRouteRequestAction = make_global<wstring>(L"ServiceRouteRequest");

GlobalWString ServiceRoutingAgentMessage::ForwardMessageAction = make_global<wstring>(L"ForwardMessage");

GlobalWString ServiceRoutingAgentMessage::ForwardToFileStoreMessageAction = make_global<wstring>(L"ForwardToFileStoreMessage");

GlobalWString ServiceRoutingAgentMessage::ForwardToTvsMessageAction = make_global<wstring>(L"ForwardToTvsMessage");

void ServiceRoutingAgentMessage::WrapForIpc(__inout Message & message)
{
    WrapForIpc(message, message.Actor, message.Action);
}

void ServiceRoutingAgentMessage::WrapForIpc(__inout Message & message, Actor::Enum const actor, wstring const & action)
{
    message.Headers.Replace(ServiceRoutingAgentProxyHeader(actor, action));
    SetRoutingAgentHeaders(message, ServiceRouteRequestAction);
}

ErrorCode ServiceRoutingAgentMessage::UnwrapFromIpc(__inout Message & message)
{
    ServiceRoutingAgentProxyHeader proxyHeader;
    if (!message.Headers.TryReadFirst(proxyHeader))
    {
        Assert::TestAssert("ServiceRoutingAgentProxyHeader missing: {0}", message.MessageId);

        return ErrorCodeValue::InvalidMessage;
    }

    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        activityHeader = FabricActivityHeader(Guid::NewGuid());

        Assert::TestAssert("FabricActivityHeader missing, generated activity {0}: {1}", activityHeader, message.MessageId);
    }

    ForwardMessageHeader forwardHeader;
    bool hasForwardHeader = message.Headers.TryReadFirst(forwardHeader);

    TimeoutHeader timeoutHeader;
    bool hasTimeoutHeader = message.Headers.TryReadFirst(timeoutHeader);

    MessageIdHeader messageIdHeader;
    bool hasMessageIdHeader = message.Headers.TryReadFirst(messageIdHeader);

    Query::QueryAddressHeader queryAddressHeader;
    bool hasQueryAddressHeader = message.Headers.TryReadFirst(queryAddressHeader);

    RequestInstanceHeader requestInstanceHeader;
    bool hasRequestInstanceHeader = message.Headers.TryReadFirst(requestInstanceHeader);

    message.Headers.RemoveAll();

    message.Headers.Replace(ActorHeader(proxyHeader.Actor));
    message.Headers.Replace(ActionHeader(proxyHeader.Action));
    message.Headers.Replace(activityHeader);

    if (hasForwardHeader) { message.Headers.Replace(forwardHeader); }
    if (hasTimeoutHeader) { message.Headers.Replace(timeoutHeader); }
    if (hasMessageIdHeader) { message.Headers.Replace(messageIdHeader); }
    if (hasQueryAddressHeader) { message.Headers.Replace(queryAddressHeader); }
    if (hasRequestInstanceHeader) { message.Headers.Replace(requestInstanceHeader); }

    return ErrorCodeValue::Success;
}

void ServiceRoutingAgentMessage::WrapForRoutingAgent(__inout Message & message, ServiceModel::ServiceTypeIdentifier const & typeId)
{
    message.Headers.Replace(ServiceRoutingAgentHeader(message.Actor, message.Action, typeId));

    SetRoutingAgentHeaders(message, ServiceRouteRequestAction);
}

void ServiceRoutingAgentMessage::WrapForRoutingAgent(__inout Client::ClientServerRequestMessage & message, ServiceModel::ServiceTypeIdentifier const & typeId)
{
    message.Headers.Replace(ServiceRoutingAgentHeader(message.Actor, message.Action, typeId));

    SetRoutingAgentHeaders(message, ServiceRouteRequestAction);
}

ErrorCode ServiceRoutingAgentMessage::RewrapForRoutingAgentProxy(__inout Message & message, ServiceRoutingAgentHeader const & routingAgentHeader)
{
    SystemServiceFilterHeader filterHeader;
    if (!message.Headers.TryReadFirst(filterHeader))
    {
        TRACE_LEVEL_AND_TESTASSERT(Trace.WriteWarning, TraceComponent, "SystemServiceFilterHeader missing {0}", message.MessageId)

            return ErrorCodeValue::InvalidMessage;
    }

    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        activityHeader = FabricActivityHeader(Guid::NewGuid());

        TRACE_LEVEL_AND_TESTASSERT(Trace.WriteInfo, TraceComponent, "SystemServiceFilterHeader missing {0}", message.MessageId)
    }

    TimeoutHeader timeoutHeader;
    bool hasTimeoutHeader = message.Headers.TryReadFirst(timeoutHeader);

    MessageIdHeader messageIdHeader;
    bool hasMessageIdHeader = message.Headers.TryReadFirst(messageIdHeader);

    Query::QueryAddressHeader queryAddressHeader;
    bool hasQueryAddressHeader = message.Headers.TryReadFirst(queryAddressHeader);

    RequestInstanceHeader requestInstanceHeader;
    bool hasRequestInstanceHeader = message.Headers.TryReadFirst(requestInstanceHeader);

    message.Headers.RemoveAll();

    message.Headers.Replace(filterHeader);
    message.Headers.Replace(activityHeader);

    if (hasTimeoutHeader) { message.Headers.Replace(timeoutHeader); }
    if (hasMessageIdHeader) { message.Headers.Replace(messageIdHeader); }
    if (hasQueryAddressHeader) { message.Headers.Replace(queryAddressHeader); }
    if (hasRequestInstanceHeader) { message.Headers.Replace(requestInstanceHeader); }

    WrapForIpc(message, routingAgentHeader.Actor, routingAgentHeader.Action);

    return ErrorCodeValue::Success;
}

void ServiceRoutingAgentMessage::WrapForForwarding(__inout Transport::Message & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardMessageAction));
}

void ServiceRoutingAgentMessage::WrapForForwardingToFileStoreService(__inout Transport::Message & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardToFileStoreMessageAction));
}

void ServiceRoutingAgentMessage::WrapForForwardingToTokenValidationService(__inout Transport::Message & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardToTvsMessageAction));
}

void ServiceRoutingAgentMessage::WrapForForwarding(__inout Client::ClientServerRequestMessage & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardMessageAction));
}

void ServiceRoutingAgentMessage::WrapForForwardingToFileStoreService(__inout Client::ClientServerRequestMessage & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardToFileStoreMessageAction));
}

void ServiceRoutingAgentMessage::WrapForForwardingToTokenValidationService(__inout Client::ClientServerRequestMessage & message)
{
    message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::NamingGateway));
    message.Headers.Replace(ActionHeader(ForwardToTvsMessageAction));
}

ErrorCode ServiceRoutingAgentMessage::ValidateIpcReply(__in Message & reply)
{
    if (reply.Action == IpcFailureAction)
    {
        IpcFailureBody body;
        if (reply.GetBody(body))
        {
            return body.TakeError();
        }
        else
        {
            return ErrorCodeValue::OperationFailed;
        }
    }
    else
    {
        return ErrorCodeValue::Success;
    }
}

MessageUPtr ServiceRoutingAgentMessage::CreateMessage(std::wstring const & action)
{
    MessageUPtr message(make_unique<Message>());

    SetRoutingAgentHeaders(*message, action);

    return message;
}

void ServiceRoutingAgentMessage::SetRoutingAgentHeaders(__inout Message & message, wstring const & action)
{
    message.Headers.Replace(ActorHeader(Actor::ServiceRoutingAgent));
    message.Headers.Replace(ActionHeader(action));
}

void ServiceRoutingAgentMessage::SetRoutingAgentHeaders(__inout Client::ClientServerRequestMessage & message, wstring const & action)
{
    message.Headers.Replace(ActorHeader(Actor::ServiceRoutingAgent));
    message.Headers.Replace(ActionHeader(action));
}

void ServiceRoutingAgentMessage::SetForwardingHeaders(
    __inout Message & message,
    Actor::Enum const actor,
    wstring const & action,
    ServiceModel::ServiceTypeIdentifier const & typeId)
{
    message.Headers.Replace(ServiceRoutingAgentHeader(actor, action, typeId));
    message.Headers.Replace(ForwardMessageHeader(Actor::ServiceRoutingAgent, ServiceRouteRequestAction));
}
