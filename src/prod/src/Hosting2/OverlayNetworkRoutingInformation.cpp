// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

StringLiteral const TraceType_OverlayNetworkRoutingInformation("OverlayNetworkRoutingInformation");

Common::WStringLiteral const OverlayNetworkRoutingInformation::NetworkNameParameter(L"NetworkName");
Common::WStringLiteral const OverlayNetworkRoutingInformation::NetworkDefinitionParameter(L"NetworkDefinition");
Common::WStringLiteral const OverlayNetworkRoutingInformation::NodeIpAddressParameter(L"InfraIPAddress");
Common::WStringLiteral const OverlayNetworkRoutingInformation::InstanceIDParameter(L"InstanceID");
Common::WStringLiteral const OverlayNetworkRoutingInformation::SequenceNumberParameter(L"SequenceNumber");
Common::WStringLiteral const OverlayNetworkRoutingInformation::IsDeltaParameter(L"IsDelta");
Common::WStringLiteral const OverlayNetworkRoutingInformation::OverlayNetworkRoutesParameter(L"Endpoints");

OverlayNetworkRoutingInformation::~OverlayNetworkRoutingInformation()
{
    WriteInfo(TraceType_OverlayNetworkRoutingInformation, "Destructing OverlayNetworkRoutingInformation.");
}

OverlayNetworkRoutingInformation::OverlayNetworkRoutingInformation(
    std::wstring networkName,
    std::wstring nodeIpAddress,
    std::wstring instanceID,
    int64 sequenceNumber,
    bool isDelta,
    std::vector<OverlayNetworkRouteSPtr> overlayNetworkRoutes) :
    networkName_(networkName),
    nodeIpAddress_(nodeIpAddress),
    instanceID_(instanceID),
    sequenceNumber_(sequenceNumber),
    isDelta_(isDelta),
    overlayNetworkRoutes_(overlayNetworkRoutes)
{
}

void OverlayNetworkRoutingInformation::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("OverlayNetworkRoutingInformation { ");
    w.Write("NetworkName = {0} ", networkName_);
    w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
    w.Write("InstanceID = {0} ", instanceID_);
    w.Write("SequenceNumber = {0} ", sequenceNumber_);
    w.Write("IsDelta = {0} ", isDelta_);
    w.Write("NetworkDefinition = {0} ", overlayNetworkDefinition_);
    w.Write("NetworkRoutes = {0} ", overlayNetworkRoutes_);
    w.Write("}");
}

