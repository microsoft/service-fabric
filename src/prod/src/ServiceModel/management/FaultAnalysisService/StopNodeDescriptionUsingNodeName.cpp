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

StringLiteral const TraceComponent("StopNodeDescriptionUsingNodeName");

StopNodeDescriptionUsingNodeName::StopNodeDescriptionUsingNodeName()
: nodeName_()
, instanceId_()
{
}

StopNodeDescriptionUsingNodeName::StopNodeDescriptionUsingNodeName(StopNodeDescriptionUsingNodeName const & other)
: nodeName_(other.nodeName_)
, instanceId_(other.instanceId_)
{
}

StopNodeDescriptionUsingNodeName::StopNodeDescriptionUsingNodeName(StopNodeDescriptionUsingNodeName && other)
: nodeName_(move(other.nodeName_))
, instanceId_(move(other.instanceId_))
{
}

Common::ErrorCode StopNodeDescriptionUsingNodeName::FromPublicApi(
    FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    instanceId_ = publicDescription.NodeInstanceId;

    return ErrorCodeValue::Success;
}

void StopNodeDescriptionUsingNodeName::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME & result) const
{
    result.NodeName = heap.AddString(nodeName_);

    result.NodeInstanceId = instanceId_;
}
