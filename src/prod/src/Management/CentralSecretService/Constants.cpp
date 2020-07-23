// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::CentralSecretService;

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::CentralSecretServicePrimaryCountName = make_global<wstring>(L"__CentralSecretServicePrimaryCount__");
GlobalWString Constants::CentralSecretServiceReplicaCountName = make_global<wstring>(L"__CentralSecretServiceReplicaCount__");
GlobalWString Constants::DatabaseDirectory = make_global<wstring>(L"CSS");
GlobalWString Constants::DatabaseFilename = make_global<wstring>(L"CSS.edb");
GlobalWString Constants::SharedLogFilename = make_global<wstring>(L"cssshared.log");

GlobalWString Constants::StoreType::Secrets = make_global<wstring>(L"Secrets");
GlobalWString Constants::StoreType::SecretsMetadata = make_global<wstring>(L"SecretsMetadata");
