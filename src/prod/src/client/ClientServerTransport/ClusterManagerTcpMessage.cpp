// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;

GlobalWString ClusterManagerTcpMessage::ProvisionApplicationTypeAction = make_global<wstring>(L"ProvisionApplicationTypeAction");
GlobalWString ClusterManagerTcpMessage::CreateApplicationAction = make_global<wstring>(L"CreateApplicationAction");
GlobalWString ClusterManagerTcpMessage::UpdateApplicationAction = make_global<wstring>(L"UpdateApplicationAction");
GlobalWString ClusterManagerTcpMessage::DeleteApplicationAction = make_global<wstring>(L"DeleteApplicationAction");
GlobalWString ClusterManagerTcpMessage::DeleteApplicationAction2 = make_global<wstring>(L"DeleteApplicationAction2");
GlobalWString ClusterManagerTcpMessage::UpgradeApplicationAction = make_global<wstring>(L"UpgradeApplicationAction");
GlobalWString ClusterManagerTcpMessage::UpdateApplicationUpgradeAction = make_global<wstring>(L"UpdateApplicationUpgradeAction");
GlobalWString ClusterManagerTcpMessage::UnprovisionApplicationTypeAction = make_global<wstring>(L"UnprovisionApplicationTypeAction");
GlobalWString ClusterManagerTcpMessage::CreateServiceAction = make_global<wstring>(L"CreateServiceAction");
GlobalWString ClusterManagerTcpMessage::CreateServiceFromTemplateAction = make_global<wstring>(L"CreateServiceFromTemplateAction");
GlobalWString ClusterManagerTcpMessage::DeleteServiceAction = make_global<wstring>(L"DeleteServiceAction");
GlobalWString ClusterManagerTcpMessage::GetUpgradeStatusAction = make_global<wstring>(L"GetUpgradeStatusAction");
GlobalWString ClusterManagerTcpMessage::ReportUpgradeHealthAction = make_global<wstring>(L"ReportUpgradeHealthAction");
GlobalWString ClusterManagerTcpMessage::MoveNextUpgradeDomainAction = make_global<wstring>(L"MoveNextUpgradeDomainAction");
GlobalWString ClusterManagerTcpMessage::ProvisionFabricAction = make_global<wstring>(L"ProvisionFabricAction");
GlobalWString ClusterManagerTcpMessage::UpgradeFabricAction = make_global<wstring>(L"UpgradeFabricAction");
GlobalWString ClusterManagerTcpMessage::UpdateFabricUpgradeAction = make_global<wstring>(L"UpdateFabricUpgradeAction");
GlobalWString ClusterManagerTcpMessage::GetFabricUpgradeStatusAction = make_global<wstring>(L"GetFabricUpgradeStatusAction");
GlobalWString ClusterManagerTcpMessage::ReportFabricUpgradeHealthAction = make_global<wstring>(L"ReportFabricUpgradeHealthAction");
GlobalWString ClusterManagerTcpMessage::MoveNextFabricUpgradeDomainAction = make_global<wstring>(L"MoveNextFabricUpgradeDomainAction");
GlobalWString ClusterManagerTcpMessage::UnprovisionFabricAction = make_global<wstring>(L"UnprovisionFabricAction");
GlobalWString ClusterManagerTcpMessage::StartInfrastructureTaskAction = make_global<wstring>(L"StartInfrastructureTaskAction");
GlobalWString ClusterManagerTcpMessage::FinishInfrastructureTaskAction = make_global<wstring>(L"FinishInfrastructureTaskAction");
GlobalWString ClusterManagerTcpMessage::RollbackApplicationUpgradeAction = make_global<wstring>(L"RollbackApplicationUpgradeAction");
GlobalWString ClusterManagerTcpMessage::RollbackFabricUpgradeAction = make_global<wstring>(L"RollbackFabricUpgradeAction");

GlobalWString ClusterManagerTcpMessage::CreateApplicationResourceAction = make_global<wstring>(L"CreateApplicationResourceAction");

const Actor::Enum ClusterManagerTcpMessage::actor_ = Actor::CM;
