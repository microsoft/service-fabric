// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TestHooks;

GlobalWString EnvironmentVariables::FabricTest_EnableTStore = make_global<wstring>(L"_FabricTest_EnableTStore");
GlobalWString EnvironmentVariables::FabricTest_EnableHashSharedLogId = make_global<wstring>(L"_FabricTest_EnableHashSharedLogId");
GlobalWString EnvironmentVariables::FabricTest_EnableSparseSharedLogs = make_global<wstring>(L"_FabricTest_EnableSparseSharedLogs");
GlobalWString EnvironmentVariables::FabricTest_SharedLogSize = make_global<wstring>(L"_FabricTest_SharedLogSize");
GlobalWString EnvironmentVariables::FabricTest_SharedLogMaximumRecordSize = make_global<wstring>(L"_FabricTest_SharedLogMaximumRecordSize");
