// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTest;

GlobalWString const MessageType::ServicePlacementData = make_global<wstring>(L"ServicePlacementData");
GlobalWString const MessageType::ServiceManifestUpdate = make_global<wstring>(L"ServiceManifestUpdate");
GlobalWString const MessageType::CodePackageActivate = make_global<wstring>(L"CodePackageActivate");
GlobalWString const MessageType::ClientCommandReply = make_global<wstring>(L"ClientCommandReply");
