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

GlobalWString TokenValidationServiceTcpMessage::GetMetadataAction = make_global<wstring>(L"GetMetadataAction");
GlobalWString TokenValidationServiceTcpMessage::ValidateTokenAction = make_global<wstring>(L"ValidateTokenAction");

const Actor::Enum TokenValidationServiceTcpMessage::actor_ = Actor::Tvs;

void TokenValidationServiceTcpMessage::WrapForTokenValidationService()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::TokenValidationServicePackageName),
            SystemServiceApplicationNameHelper::TokenValidationServiceType));
}
