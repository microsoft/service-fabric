// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

Common::WStringLiteral const NetworkConfig::NetworkName(L"Name");
Common::WStringLiteral const NetworkConfig::Driver(L"Driver");
Common::WStringLiteral const NetworkConfig::CheckDuplicate(L"CheckDuplicate");
Common::WStringLiteral const NetworkConfig::NetworkIPAM(L"Ipam");

NetworkConfig::NetworkConfig()
{
}

NetworkConfig::NetworkConfig(
    std::wstring name,
    std::wstring driver,
    bool checkDuplicate,
    Hosting2::NetworkIPAM networkIPAM) :
    name_(name),
    driver_(driver),
    checkDuplicate_(checkDuplicate),
    networkIPAM_(networkIPAM)
{
}
