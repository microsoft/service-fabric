// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::GatewayResourceManager;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::GatewayResourceManagerPrimaryCountName = make_global<wstring>(L"__GatewayResourceManagerPrimaryCount__");
GlobalWString Constants::GatewayResourceManagerReplicaCountName = make_global<wstring>(L"__GatewayResourceManagerReplicaCount__");
GlobalWString Constants::CreateGatewayResourceAction = make_global<wstring>(L"CreateGatewayResourceAction");
GlobalWString Constants::DeleteGatewayResourceAction = make_global<wstring>(L"DeleteGatewayResourceAction");
GlobalWString Constants::QueryGatewayResourceAction = make_global<wstring>(L"QueryGatewayResourceAction");
