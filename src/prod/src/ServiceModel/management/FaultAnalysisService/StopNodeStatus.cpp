// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("StopNodeStatus");

StopNodeStatus::StopNodeStatus()
: nodeResult_()
{
}

StopNodeStatus::StopNodeStatus(
    shared_ptr<Management::FaultAnalysisService::NodeResult> && nodeResult)
    : nodeResult_(std::move(nodeResult))
{
}

StopNodeStatus::StopNodeStatus(StopNodeStatus && other)
: nodeResult_(move(other.nodeResult_))
{
}

StopNodeStatus & StopNodeStatus::operator=(StopNodeStatus && other)
{
    if (this != &other)
    {
        nodeResult_ = move(other.nodeResult_);
    }

    return *this;
}

shared_ptr<NodeResult> const & StopNodeStatus::GetNodeResult()
{
    return nodeResult_;
}

Common::ErrorCode StopNodeStatus::FromPublicApi(__in FABRIC_STOP_NODE_STATUS const & publicResult)
{
    NodeResult nodeResult;
    auto error = nodeResult.FromPublicApi(*publicResult.NodeResult);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "StopNodeStatus::FromPublicApi/Failed at nodeResult.FromPublicApi");
        return error;
    }

    nodeResult_ = make_shared<NodeResult>(nodeResult);

    return ErrorCodeValue::Success;
}

void StopNodeStatus::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_STOP_NODE_STATUS & publicResult) const
{
    if (nodeResult_)
    {
        auto resultPtr = heap.AddItem<FABRIC_NODE_RESULT>();

        nodeResult_->ToPublicApi(heap, *resultPtr);

        publicResult.NodeResult = resultPtr.GetRawPointer();
    }
}
