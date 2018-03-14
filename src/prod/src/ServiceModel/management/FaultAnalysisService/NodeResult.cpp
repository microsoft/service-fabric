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

StringLiteral const TraceComponent("NodeResult");

NodeResult::NodeResult()
    :nodeName_()
    , nodeInstanceId_()
{
}

NodeResult::NodeResult(NodeResult && other)
    :nodeName_(move(other.nodeName_))
    , nodeInstanceId_(move(other.nodeInstanceId_))
{
}

ErrorCode NodeResult::FromPublicApi(FABRIC_NODE_RESULT const & publicResult)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicResult.NodeName, true, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "NodeResult::FromPublicApi/Failed to parse NodeName");
        return ErrorCode::FromHResult(hr);
    }

    nodeInstanceId_ = publicResult.NodeInstance;

    return ErrorCodeValue::Success;
}

void NodeResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_NODE_RESULT & publicResult) const
{
    publicResult.NodeName = heap.AddString(nodeName_);

    publicResult.NodeInstance = nodeInstanceId_;
}

