// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

Common::WStringLiteral const NetworkIPAM::Config(L"Config");

NetworkIPAM::NetworkIPAM()
{
}

NetworkIPAM::NetworkIPAM(
    std::vector<Hosting2::NetworkIPAMConfig> ipamConfig) :
    ipamConfig_(ipamConfig)
{
}
