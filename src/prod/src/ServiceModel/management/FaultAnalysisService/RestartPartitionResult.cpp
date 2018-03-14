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

StringLiteral const TraceComponent("RestartPartitionResult");

RestartPartitionResult::RestartPartitionResult()
    : errorCode_(0)
    , selectedPartition_()
{
}

RestartPartitionResult::RestartPartitionResult(RestartPartitionResult && other)
    : errorCode_(move(other.errorCode_))
    , selectedPartition_(move(other.selectedPartition_))
{
}

ErrorCode RestartPartitionResult::FromPublicApi(FABRIC_PARTITION_RESTART_RESULT const & publicResult)
{
    errorCode_ = publicResult.ErrorCode;

    SelectedPartition selectedPartition;

    FABRIC_SELECTED_PARTITION * publicSelectedPartition = publicResult.SelectedPartition;

    auto error = selectedPartition.FromPublicApi(*publicSelectedPartition);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "RestartPartitionResult::FromPublicApi/Failed at selectedPartition.FromPublicApi");
        return error;
    }

    selectedPartition_ = make_shared<SelectedPartition>(selectedPartition);

    return ErrorCodeValue::Success;
}

void RestartPartitionResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_PARTITION_RESTART_RESULT & publicResult) const
{
    publicResult.ErrorCode = errorCode_;

    auto publicSelectedPartitionPtr = heap.AddItem<FABRIC_SELECTED_PARTITION>();
    selectedPartition_->ToPublicApi(heap, *publicSelectedPartitionPtr);
    publicResult.SelectedPartition = publicSelectedPartitionPtr.GetRawPointer();
}

