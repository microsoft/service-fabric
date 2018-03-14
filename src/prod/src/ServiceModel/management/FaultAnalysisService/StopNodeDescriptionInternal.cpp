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

StringLiteral const TraceComponent("StopNodeDescriptionInternal");

StopNodeDescriptionInternal::StopNodeDescriptionInternal()
: nodeName_()
, instanceId_()
, stopDurationInSeconds_()
{
}
#if 0
StopNodeDescriptionInternal::StopNodeDescriptionInternal(StopNodeDescriptionInternal const & other)
: nodeName_(other.nodeName_)
, instanceId_(other.instanceId_)
, stopDurationInSeconds_(other.stopDurationInSeconds_)
{
}
#endif
StopNodeDescriptionInternal::StopNodeDescriptionInternal(StopNodeDescriptionInternal && other)
: nodeName_(move(other.nodeName_))
, instanceId_(move(other.instanceId_))
, stopDurationInSeconds_(move(other.stopDurationInSeconds_))
{
}

Common::ErrorCode StopNodeDescriptionInternal::FromPublicApi(
    FABRIC_STOP_NODE_DESCRIPTION_INTERNAL const & publicDescription)
{
    Trace.WriteInfo(TraceComponent, "Inside StopNodeDescriptionInternal::FromPublicApi");

    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    instanceId_ = publicDescription.NodeInstanceId;

    Trace.WriteInfo(TraceComponent, "Inside StopNodeDescriptionInternal::FromPublicApi duration is '{0}'", publicDescription.StopDurationInSeconds);

    stopDurationInSeconds_ = publicDescription.StopDurationInSeconds;
    
    return ErrorCodeValue::Success;
}

void StopNodeDescriptionInternal::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_STOP_NODE_DESCRIPTION_INTERNAL & result) const
{
    Trace.WriteInfo(TraceComponent, "Inside StopNodeDescriptionInternal::FromPublicApi");

    result.NodeName = heap.AddString(nodeName_);

    result.NodeInstanceId = instanceId_;

    Trace.WriteInfo(TraceComponent, "Inside StopNodeDescriptionInternal::FromPublicApi duration is '{0}'", stopDurationInSeconds_);

    result.StopDurationInSeconds = stopDurationInSeconds_;
}
