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
using namespace ClientServerTransport;
using namespace Naming;

GlobalWString GatewayResourceManagerTcpMessage::CreateGatewayResourceAction = make_global<wstring>(L"CreateGatewayResourceAction");
GlobalWString GatewayResourceManagerTcpMessage::DeleteGatewayResourceAction = make_global<wstring>(L"DeleteGatewayResourceAction");

const Actor::Enum GatewayResourceManagerTcpMessage::actor_ = Actor::GatewayResourceManager;

void GatewayResourceManagerTcpMessage::WrapForGatewayResourceManager()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        *ServiceTypeIdentifier::GatewayResourceManagerServiceTypeId);

    // Add ServiceTargetHeader for entreeservice to resolve this service.
    NamingUri serviceName;
    NamingUri::TryParse(*SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName, serviceName);
    this->Headers.Replace(ServiceTargetHeader(serviceName));
}
