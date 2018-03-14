// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

using namespace Naming;

void GatewayDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << "[ ";
    
    if (!address_.empty())
    {
        w << "address=" << address_ << " ";
    }
    
    w << "nodeInstance=" << nodeInstance_ << " ";

    if (!nodeName_.empty())
    {
        w << "nodeName=" << nodeName_ << " ";
    }

    w << "]"; 
}

void GatewayDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_GATEWAY_INFORMATION & result) const
{
    result.NodeAddress = heap.AddString(address_);
    nodeInstance_.Id.ToPublicApi(result.NodeId);
    result.NodeInstanceId = nodeInstance_.InstanceId;
    result.NodeName = heap.AddString(nodeName_);
}
