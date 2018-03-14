// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("StartNodeStatus");

StartNodeStatus::StartNodeStatus()
: nodeResult_()
{
}

StartNodeStatus::StartNodeStatus(
    shared_ptr<Management::FaultAnalysisService::NodeResult> && nodeResult)
    : nodeResult_(std::move(nodeResult))
{
}

StartNodeStatus::StartNodeStatus(StartNodeStatus && other)
: nodeResult_(move(other.nodeResult_))
{
}

StartNodeStatus & StartNodeStatus::operator=(StartNodeStatus && other)
{
    if (this != &other)
    {
        nodeResult_ = move(other.nodeResult_);
    }

    return *this;
}

shared_ptr<NodeResult> const & StartNodeStatus::GetNodeResult()
{
    return nodeResult_;
}

Common::ErrorCode StartNodeStatus::FromPublicApi(__in FABRIC_START_NODE_STATUS const & publicResult)
{
    NodeResult nodeResult;
    auto error = nodeResult.FromPublicApi(*publicResult.NodeResult);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "StartNodeStatus::FromPublicApi/Failed at nodeResult.FromPublicApi");
        return error;
    }

    nodeResult_ = make_shared<NodeResult>(nodeResult);

    return ErrorCodeValue::Success;
}

void StartNodeStatus::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_START_NODE_STATUS & publicResult) const
{
    if (nodeResult_)
    {
        auto resultPtr = heap.AddItem<FABRIC_NODE_RESULT>();

        nodeResult_->ToPublicApi(heap, *resultPtr);

        publicResult.NodeResult = resultPtr.GetRawPointer();
    }
}
