// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace std;

ServicePlacementData::ServicePlacementData(
    map<std::wstring, TestServiceInfo> const& placedServices, 
    TestCodePackageContext const& codePackageContext)
    : codePackageContext_(codePackageContext),
    placedServices_(placedServices)
{
}
