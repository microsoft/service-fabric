// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

GetDeployedNetworksReply::GetDeployedNetworksReply()
: networkNames_()
{
}

GetDeployedNetworksReply::GetDeployedNetworksReply(
    std::vector<std::wstring> const & networkNames)
    : networkNames_(networkNames)
{
}

void GetDeployedNetworksReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetDeployedNetworksReply { ");
    for (auto const & networkName : networkNames_)
    {
        w.Write("NetworkName = {0} ", networkName);
    }
    w.Write("}");
}
