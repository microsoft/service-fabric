// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;

// Keep the string as old name for backward compatibility
GlobalWString ContainerOperationTcpMessage::CreateComposeDeploymentAction = make_global<wstring>(L"CreateDockerComposeApplicationAction");
GlobalWString ContainerOperationTcpMessage::DeleteComposeDeploymentAction = make_global<wstring>(L"DeleteDockerComposeApplicationAction");
GlobalWString ContainerOperationTcpMessage::UpgradeComposeDeploymentAction = make_global<wstring>(L"UpgradeComposeDeploymentAction");
GlobalWString ContainerOperationTcpMessage::RollbackComposeDeploymentUpgradeAction = make_global<wstring>(L"RollbackComposeDeploymentUpgradeAction");
GlobalWString ContainerOperationTcpMessage::DeleteSingleInstanceDeploymentAction = make_global<wstring>(L"DeleteSingleInstanceDeploymentAction");

const Actor::Enum ContainerOperationTcpMessage::actor_ = Actor::CM;
