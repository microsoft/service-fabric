// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("InvokeDataLossProgress");

InvokeDataLossProgress::InvokeDataLossProgress()
    : state_(TestCommandProgressState::Invalid)
    , result_()
{ 
}

InvokeDataLossProgress::InvokeDataLossProgress(InvokeDataLossProgress && other) 
    : state_(move(other.state_))
    , result_(move(other.result_))
{ 
}

InvokeDataLossProgress & InvokeDataLossProgress::operator=(InvokeDataLossProgress && other) 
{
    if (this != &other)
    {
        state_ = move(other.state_);
        result_ = move(other.result_);
    }

    return *this;
}

InvokeDataLossProgress::InvokeDataLossProgress(
    TestCommandProgressState::Enum state,
    std::shared_ptr<InvokeDataLossResult> && result)
    : state_(state)
    , result_(move(result))
{ 
}

Common::ErrorCode InvokeDataLossProgress::FromPublicApi(__in FABRIC_PARTITION_DATA_LOSS_PROGRESS const & publicProgress)
{
    state_ = TestCommandProgressState::FromPublicApi(publicProgress.State);

    InvokeDataLossResult result;
    auto error = result.FromPublicApi(*publicProgress.Result);

    if (!error.IsSuccess())
    {
        return error;
    }

    result_ = make_shared<InvokeDataLossResult>(result);

    return ErrorCodeValue::Success;
}

void InvokeDataLossProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PARTITION_DATA_LOSS_PROGRESS & progress) const
{
    if (result_)
    {
        auto resultPtr = heap.AddItem<FABRIC_PARTITION_DATA_LOSS_RESULT>();
        result_->ToPublicApi(heap, *resultPtr);
        progress.Result = resultPtr.GetRawPointer();
    }

    progress.State = TestCommandProgressState::ToPublicApi(state_);
}
