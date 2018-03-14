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
using namespace Management::InfrastructureService;

GlobalWString InfrastructureServiceMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");

void InfrastructureServiceMessage::WrapForInfrastructureService(__inout Message & message)
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        message,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::InfrastructureServicePackageName),
            SystemServiceApplicationNameHelper::InfrastructureServiceType));
}

void InfrastructureServiceMessage::SetHeaders(Message & message, wstring const & action)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(Actor::IS));
    message.Headers.Add(FabricActivityHeader(Guid::NewGuid()));
}
