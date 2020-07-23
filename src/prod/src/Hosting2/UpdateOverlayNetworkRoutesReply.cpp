// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Reliability;

UpdateOverlayNetworkRoutesReply::UpdateOverlayNetworkRoutesReply(
    std::wstring const & networkName,
    std::wstring const & nodeIpAddress,
    Common::ErrorCode const & error) :
    networkName_(networkName),
    nodeIpAddress_(nodeIpAddress),
    error_(error)
{
}

UpdateOverlayNetworkRoutesReply::~UpdateOverlayNetworkRoutesReply()
{
}

void UpdateOverlayNetworkRoutesReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UpdateOverlayNetworkRoutesReply { ");
    w.Write("NetworkName = {0} ", networkName_);
    w.Write("NodeIpAddres = {0} ", nodeIpAddress_);
    w.Write("Error = {0} ", error_);
    w.Write("}");
}
