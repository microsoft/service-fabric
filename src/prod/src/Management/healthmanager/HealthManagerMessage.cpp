// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Management::HealthManager;

GlobalWString HealthManagerMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");

void HealthManagerMessage::SetHeaders(
    __in Message & message, 
    wstring const & action,
    Common::ActivityId const & activityId)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(Actor::HM));
    message.Headers.Add(FabricActivityHeader(activityId));
}

Transport::MessageUPtr HealthManagerMessage::CreateMessage(
    std::wstring const & action,
    Common::ActivityId const & activityId)
{
    Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
    HealthManagerMessage::SetHeaders(*message, action, activityId);
    return message;
}
