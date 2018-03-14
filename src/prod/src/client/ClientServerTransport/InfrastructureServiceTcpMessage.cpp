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

GlobalWString InfrastructureServiceTcpMessage::RunCommandAction = make_global<wstring>(L"RunCommandAction");
GlobalWString InfrastructureServiceTcpMessage::InvokeInfrastructureCommandAction = make_global<wstring>(L"InvokeInfrastructureCommandAction");
GlobalWString InfrastructureServiceTcpMessage::InvokeInfrastructureQueryAction = make_global<wstring>(L"InvokeInfrastructureQueryAction");
GlobalWString InfrastructureServiceTcpMessage::ReportStartTaskSuccessAction = make_global<wstring>(L"ReportStartTaskSuccessAction");
GlobalWString InfrastructureServiceTcpMessage::ReportFinishTaskSuccessAction = make_global<wstring>(L"ReportFinishTaskSuccessAction");
GlobalWString InfrastructureServiceTcpMessage::ReportTaskFailureAction = make_global<wstring>(L"ReportTaskFailureAction");

void InfrastructureServiceTcpMessage::WrapForInfrastructureService()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::InfrastructureServicePackageName),
            SystemServiceApplicationNameHelper::InfrastructureServiceType));
}

const Actor::Enum InfrastructureServiceTcpMessage::actor_ = Actor::IS;
