// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::NetworkInventoryManager;

const std::wstring PublishNetworkTablesRequestMessage::ActionName = L"PublishNetworkTablesRequest";

void PublishNetworkTablesRequestMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.WriteLine("PublishNetworkTablesRequestMessage: Network: [{0}], InstanceID: [{1}], Nodes: [", networkName_, instanceID_);
    for(auto i : nodeInstances_) { w.WriteLine(" {0} ", i.ToString()); }
    w.WriteLine("], Endpoints: [");
    for(auto nae : networkMappingTable_) {  w.WriteLine("( {0}, {1}, {2} )", nae->IpAddress, nae->InfraIpAddress, nae->MacAddress); }
    w.WriteLine("] seq: {0} ", sequenceNumber_);
    w.WriteLine("delta: {0} ", isDelta_);
}
