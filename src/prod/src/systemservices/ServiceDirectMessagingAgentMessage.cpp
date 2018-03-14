// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace SystemServices;

StringLiteral const TraceComponent("ServiceDirectMessagingAgentMessage");

GlobalWString ServiceDirectMessagingAgentMessage::DirectMessagingFailureAction = make_global<wstring>(L"DirectMessagingFailure");

GlobalWString ServiceDirectMessagingAgentMessage::DirectMessagingAction = make_global<wstring>(L"DirectMessaging");

void ServiceDirectMessagingAgentMessage::WrapServiceRequest(__inout Message & message, SystemServiceLocation const & primaryLocation)
{
    message.Headers.Replace(ServiceDirectMessagingHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::DirectMessagingAgent));
    message.Headers.Replace(ActionHeader(DirectMessagingAction));
    message.Headers.Replace(primaryLocation.CreateFilterHeader());
}

void ServiceDirectMessagingAgentMessage::WrapServiceRequest(__inout Client::ClientServerRequestMessage & message, SystemServiceLocation const & primaryLocation)
{
    message.Headers.Replace(ServiceDirectMessagingHeader(message.Actor, message.Action));
    message.Headers.Replace(ActorHeader(Actor::DirectMessagingAgent));
    message.Headers.Replace(ActionHeader(DirectMessagingAction));
    message.Headers.Replace(primaryLocation.CreateFilterHeader());
}

ErrorCode ServiceDirectMessagingAgentMessage::UnwrapFromTransport(__inout Message & message)
{
    ServiceDirectMessagingHeader dmHeader;
    if (!message.Headers.TryReadFirst(dmHeader))
    {
        Assert::TestAssert("ServiceDirectMessagingHeader missing: {0}", message.MessageId);

        return ErrorCodeValue::InvalidMessage;
    }

    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        activityHeader = FabricActivityHeader(Guid::NewGuid());

        Assert::TestAssert("FabricActivityHeader missing, generated activity {0}: {1}", activityHeader, message.MessageId);

        message.Headers.Add(activityHeader);
    }

    message.Headers.Replace(ActorHeader(dmHeader.Actor));
    message.Headers.Replace(ActionHeader(dmHeader.Action));

    return ErrorCodeValue::Success;
}

ErrorCode ServiceDirectMessagingAgentMessage::UnwrapServiceReply(__inout Message & message)
{
    if (message.Action == ServiceDirectMessagingAgentMessage::DirectMessagingFailureAction)
    {
        DirectMessagingFailureBody body;
        if (message.GetBody(body))
        {
            return body.ErrorDetails.TakeError();
        }
        else
        {
            return ErrorCode::FromNtStatus(message.GetStatus());
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceDirectMessagingAgentMessage::UnwrapServiceReply(__inout Client::ClientServerReplyMessage & message)
{
    if (message.Action == ServiceDirectMessagingAgentMessage::DirectMessagingFailureAction)
    {
        DirectMessagingFailureBody body;
        if (message.GetBody(body))
        {
            return body.ErrorDetails.TakeError();
        }
        else
        {
            return ErrorCode::FromNtStatus(message.GetStatus());
        }
    }

    return ErrorCodeValue::Success;
}
