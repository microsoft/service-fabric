// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Management::GatewayResourceManager;
GlobalWString GatewayResourceManagerMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");

void GatewayResourceManagerMessage::WrapForGatewayResourceManager(__inout Message & message)
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        message,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::GatewayResourceManagerPackageName),
            SystemServiceApplicationNameHelper::GatewayResourceManagerType));
}

void GatewayResourceManagerMessage::SetHeaders(Message & message, wstring const & action)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(Actor::GatewayResourceManager));
    message.Headers.Add(FabricActivityHeader(Guid::NewGuid()));
}
