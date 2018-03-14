// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::RepairManager;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::RepairManagerServicePrimaryCountName = make_global<wstring>(L"__RepairManagerServicePrimaryCount__");
GlobalWString Constants::RepairManagerServiceReplicaCountName = make_global<wstring>(L"__RepairManagerServiceReplicaCount__");

// -------------------------------
// Store Data Keys
// -------------------------------

GlobalWString Constants::DatabaseDirectory = make_global<wstring>(L"RM");
GlobalWString Constants::DatabaseFilename = make_global<wstring>(L"RM.edb");
GlobalWString Constants::SharedLogFilename = make_global<wstring>(L"rmshared.log");

GlobalWString Constants::StoreType_RepairTaskContext = make_global<wstring>(L"RepairTaskContext");
GlobalWString Constants::StoreType_HealthCheckStoreData = make_global<wstring>(L"HealthCheckStoreData");
GlobalWString Constants::StoreKey_HealthCheckStoreData = make_global<wstring>(L"HealthCheckStoreDataKey");
