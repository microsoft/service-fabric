// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

StringLiteral const TraceType_OverlayNetworkRoute("OverlayNetworkRoute");

Common::WStringLiteral const OverlayNetworkRoute::IPV4Address(L"IPV4Address");
Common::WStringLiteral const OverlayNetworkRoute::NodeIpAddress(L"NodeIpAddress");
Common::WStringLiteral const OverlayNetworkRoute::MacAddress(L"MacAddress");

OverlayNetworkRoute::OverlayNetworkRoute()
{
}

OverlayNetworkRoute::~OverlayNetworkRoute() 
{
    WriteInfo(TraceType_OverlayNetworkRoute, "Destructing OverlayNetworkRoute.");
}

OverlayNetworkRoute::OverlayNetworkRoute(
    std::wstring ipAddress,
    std::wstring macAddress,
    std::wstring nodeIpAddress) :
    ipAddress_(ipAddress),
    macAddress_(macAddress),
    nodeIpAddress_(nodeIpAddress)
{
}

void OverlayNetworkRoute::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("OverlayNetworkRoute { ");
    w.Write("IpAddress = {0} ", ipAddress_);
    w.Write("MacAddress = {0} ", macAddress_);
    w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
    w.Write("}");
}