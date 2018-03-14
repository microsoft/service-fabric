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

GlobalWString RepairManagerTcpMessage::CreateRepairRequestAction = make_global<wstring>(L"CreateRepairRequestAction");
GlobalWString RepairManagerTcpMessage::CancelRepairRequestAction = make_global<wstring>(L"CancelRepairRequestAction");
GlobalWString RepairManagerTcpMessage::ForceApproveRepairAction = make_global<wstring>(L"ForceApproveRepairAction");
GlobalWString RepairManagerTcpMessage::DeleteRepairRequestAction = make_global<wstring>(L"DeleteRepairRequestAction");
GlobalWString RepairManagerTcpMessage::UpdateRepairExecutionStateAction = make_global<wstring>(L"UpdateRepairExecutionStateAction");
GlobalWString RepairManagerTcpMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");
GlobalWString RepairManagerTcpMessage::UpdateRepairTaskHealthPolicyAction = make_global<wstring>(L"UpdateRepairTaskHealthPolicyAction");

const Actor::Enum RepairManagerTcpMessage::actor_ = Actor::RM;

void RepairManagerTcpMessage::WrapForRepairManager()
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(
        *this,
        *ServiceTypeIdentifier::RepairManagerServiceTypeId);
}
