// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("RestartPartitionProgress");

RestartPartitionProgress::RestartPartitionProgress()
: state_(TestCommandProgressState::Invalid)
, result_()
{
}

RestartPartitionProgress::RestartPartitionProgress(RestartPartitionProgress && other)
: state_(move(other.state_))
, result_(move(other.result_))
{
}

RestartPartitionProgress & RestartPartitionProgress::operator=(RestartPartitionProgress && other)
{
    if (this != &other)
    {
        state_ = move(other.state_);
        result_ = move(other.result_);
    }

    return *this;
}

RestartPartitionProgress::RestartPartitionProgress(
    TestCommandProgressState::Enum state,
    std::shared_ptr<RestartPartitionResult> && result)
    : state_(state)
    , result_(move(result))
{
}

Common::ErrorCode RestartPartitionProgress::FromPublicApi(__in FABRIC_PARTITION_RESTART_PROGRESS const & publicProgress)
{
    state_ = TestCommandProgressState::FromPublicApi(publicProgress.State);

    RestartPartitionResult result;
    auto error = result.FromPublicApi(*publicProgress.Result);

    if (!error.IsSuccess())
    {
        return error;
    }

    result_ = make_shared<RestartPartitionResult>(result);

    return ErrorCodeValue::Success;
}

void RestartPartitionProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PARTITION_RESTART_PROGRESS & progress) const
{        
    if (result_)
    {
        auto resultPtr = heap.AddItem<FABRIC_PARTITION_RESTART_RESULT>();
        result_->ToPublicApi(heap, *resultPtr);
        progress.Result = resultPtr.GetRawPointer();
    }

    progress.State = TestCommandProgressState::ToPublicApi(state_);
}

