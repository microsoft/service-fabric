// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

Common::WStringLiteral const NetworkIPAMConfig::Subnet(L"Subnet");
Common::WStringLiteral const NetworkIPAMConfig::Gateway(L"Gateway");

NetworkIPAMConfig::NetworkIPAMConfig()
{
}

NetworkIPAMConfig::NetworkIPAMConfig(
    std::wstring subnet,
    std::wstring gateway) :
    subnet_(subnet),
    gateway_(gateway)
{
}
