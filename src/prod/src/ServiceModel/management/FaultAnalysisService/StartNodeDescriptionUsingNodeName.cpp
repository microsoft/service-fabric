// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("StartNodeDescriptionUsingNodeName");

StartNodeDescriptionUsingNodeName::StartNodeDescriptionUsingNodeName()
: nodeName_()
, instanceId_()
, ipAddressOrFQDN_()
, clusterConnectionPort_()
{
}

StartNodeDescriptionUsingNodeName::StartNodeDescriptionUsingNodeName(StartNodeDescriptionUsingNodeName const & other)
: nodeName_(other.nodeName_)
, instanceId_(other.instanceId_)
, ipAddressOrFQDN_(other.ipAddressOrFQDN_)
, clusterConnectionPort_(other.clusterConnectionPort_)
{
}

StartNodeDescriptionUsingNodeName::StartNodeDescriptionUsingNodeName(StartNodeDescriptionUsingNodeName && other)
: nodeName_(move(other.nodeName_))
, instanceId_(move(other.instanceId_))
, ipAddressOrFQDN_(move(other.ipAddressOrFQDN_))
, clusterConnectionPort_(move(other.clusterConnectionPort_))
{
}

Common::ErrorCode StartNodeDescriptionUsingNodeName::FromPublicApi(
    FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    instanceId_ = publicDescription.NodeInstanceId;

    hr = StringUtility::LpcwstrToWstring(publicDescription.IPAddressOrFQDN, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, ipAddressOrFQDN_);
    if (FAILED(hr)) { Trace.WriteWarning(TraceComponent, "Failed parsing ip"); return ErrorCodeValue::InvalidNameUri; }

    clusterConnectionPort_ = publicDescription.ClusterConnectionPort;

    return ErrorCodeValue::Success;
}

void StartNodeDescriptionUsingNodeName::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME & result) const
{
    result.NodeName = heap.AddString(nodeName_);

    result.NodeInstanceId = instanceId_;

    result.IPAddressOrFQDN = heap.AddString(ipAddressOrFQDN_);

    result.ClusterConnectionPort = clusterConnectionPort_;
}
