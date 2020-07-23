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

UpdateOverlayNetworkRoutesRequest::UpdateOverlayNetworkRoutesRequest(
    std::wstring networkName,
    std::wstring nodeIpAddress,
    std::wstring instanceID,
    int64 sequenceNumber,
    bool isDelta,
    std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr> networkMappingTable) :
    networkName_(networkName),
    nodeIpAddress_(nodeIpAddress),
    instanceID_(instanceID),
    sequenceNumber_(sequenceNumber),
    isDelta_(isDelta),
    networkMappingTable_(networkMappingTable)
{
}

UpdateOverlayNetworkRoutesRequest::~UpdateOverlayNetworkRoutesRequest()
{
}

void UpdateOverlayNetworkRoutesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UpdateOverlayNetworkRoutesRequest { ");
    w.Write("NetworkName = {0} ", networkName_);
    w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
    w.Write("InstanceID = {0} ", instanceID_);
    w.Write("SequenceNumber = {0} ", sequenceNumber_);
    w.Write("IsDelta = {0} ", isDelta_);
    w.Write("}");
}
