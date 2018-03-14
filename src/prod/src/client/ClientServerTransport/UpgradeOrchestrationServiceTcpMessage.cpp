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

GlobalWString UpgradeOrchestrationServiceTcpMessage::StartClusterConfigurationUpgradeAction = make_global<wstring>(L"StartClusterConfigurationUpgradeAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::GetClusterConfigurationUpgradeStatusAction = make_global<wstring>(L"GetClusterConfigurationUpgradeStatusAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::GetClusterConfigurationAction = make_global<wstring>(L"GetClusterConfigurationAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::GetUpgradesPendingApprovalAction = make_global<wstring>(L"GetUpgradesPendingApprovalAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::StartApprovedUpgradesAction = make_global<wstring>(L"StartApprovedUpgradesAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::GetUpgradeOrchestrationServiceStateAction = make_global<wstring>(L"GetUpgradeOrchestrationServiceStateAction");
GlobalWString UpgradeOrchestrationServiceTcpMessage::SetUpgradeOrchestrationServiceStateAction = make_global<wstring>(L"SetUpgradeOrchestrationServiceStateAction");

void UpgradeOrchestrationServiceTcpMessage::WrapForUpgradeOrchestrationService()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        ServiceTypeIdentifier(
            ServicePackageIdentifier(
                ApplicationIdentifier::FabricSystemAppId->ToString(),
                SystemServiceApplicationNameHelper::UpgradeOrchestrationServicePackageName),
            SystemServiceApplicationNameHelper::UpgradeOrchestrationServiceType));
}

const Actor::Enum UpgradeOrchestrationServiceTcpMessage::actor_ = Actor::UOS;
