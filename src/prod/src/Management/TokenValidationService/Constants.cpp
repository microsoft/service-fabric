// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::TokenValidationService;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::TokenValidationServicePrimaryCountName = make_global<wstring>(L"__TokenValidationServicePrimaryCount__");
GlobalWString Constants::TokenValidationServiceReplicaCountName = make_global<wstring>(L"__TokenValidationServiceReplicaCount__");
GlobalWString Constants::TokenValidationAction = make_global<wstring>(L"TokenValidationAction");
GlobalWString Constants::GetMetadataAction = make_global<wstring>(L"GetMetadataAction");
