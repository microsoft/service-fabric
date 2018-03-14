// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("InvokeQuorumLossProgress");

InvokeQuorumLossProgress::InvokeQuorumLossProgress()
    : state_(TestCommandProgressState::Invalid)
    , result_()
{
}

InvokeQuorumLossProgress::InvokeQuorumLossProgress(InvokeQuorumLossProgress && other) 
    : state_(move(other.state_))
    , result_(move(other.result_))
{
}

InvokeQuorumLossProgress & InvokeQuorumLossProgress::operator=(InvokeQuorumLossProgress && other) 
{
    if (this != &other)
    {
        state_ = move(other.state_);
        result_ = move(other.result_);
    }

    return *this;
}

InvokeQuorumLossProgress::InvokeQuorumLossProgress(
    TestCommandProgressState::Enum state,
    std::shared_ptr<InvokeQuorumLossResult> && result)
    : state_(state)
    , result_(move(result))  
{ 
}

Common::ErrorCode InvokeQuorumLossProgress::FromPublicApi(__in FABRIC_PARTITION_QUORUM_LOSS_PROGRESS const & publicProgress)
{
    state_ = TestCommandProgressState::FromPublicApi(publicProgress.State);

    InvokeQuorumLossResult result;
    auto error = result.FromPublicApi(*publicProgress.Result);

    if (!error.IsSuccess())
    {
        return error;
    }

    result_ = make_shared<InvokeQuorumLossResult>(result);

    return ErrorCodeValue::Success;
}

void InvokeQuorumLossProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PARTITION_QUORUM_LOSS_PROGRESS & progress) const
{
    if (result_)
    {
        auto resultPtr = heap.AddItem<FABRIC_PARTITION_QUORUM_LOSS_RESULT>();
        result_->ToPublicApi(heap, *resultPtr);
        progress.Result = resultPtr.GetRawPointer();
    }

    progress.State = TestCommandProgressState::ToPublicApi(state_);
}
