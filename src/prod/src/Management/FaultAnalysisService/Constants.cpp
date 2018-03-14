// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FaultAnalysisService;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::FaultAnalysisServicePrimaryCountName = make_global<wstring>(L"__FaultAnalysisServicePrimaryCount__");
GlobalWString Constants::FaultAnalysisServiceReplicaCountName = make_global<wstring>(L"__FaultAnalysisServiceReplicaCount__");
