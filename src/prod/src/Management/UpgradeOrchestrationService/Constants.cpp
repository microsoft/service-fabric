// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::UpgradeOrchestrationService;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::UpgradeOrchestrationServicePrimaryCountName = make_global<wstring>(L"__UpgradeOrchestrationServicePrimaryCount__");
GlobalWString Constants::UpgradeOrchestrationServiceReplicaCountName = make_global<wstring>(L"__UpgradeOrchestrationServiceReplicaCount__");
