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

StringLiteral const TraceComponent("InvokeQuorumLossResult");

InvokeQuorumLossResult::InvokeQuorumLossResult() 
    : errorCode_(0)
    , selectedPartition_()
{ 
}

InvokeQuorumLossResult::InvokeQuorumLossResult(InvokeQuorumLossResult && other) 
    : errorCode_(move(other.errorCode_))
    , selectedPartition_(move(other.selectedPartition_))
{ 
}

ErrorCode InvokeQuorumLossResult::FromPublicApi(FABRIC_PARTITION_QUORUM_LOSS_RESULT const & publicResult)
{
    errorCode_ = publicResult.ErrorCode;
    SelectedPartition selectedPartition;
    FABRIC_SELECTED_PARTITION * publicSelectedPartition = publicResult.SelectedPartition;
    auto error = selectedPartition.FromPublicApi(*publicSelectedPartition);

    if (!error.IsSuccess())
    {
        return error;
    }

    selectedPartition_ = make_shared<SelectedPartition>(selectedPartition);
    return ErrorCodeValue::Success;
}

void InvokeQuorumLossResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_PARTITION_QUORUM_LOSS_RESULT & publicResult) const
{
    publicResult.ErrorCode = errorCode_;
    auto publicSelectedPartitionPtr = heap.AddItem<FABRIC_SELECTED_PARTITION>();
    selectedPartition_->ToPublicApi(heap, *publicSelectedPartitionPtr);
    publicResult.SelectedPartition = publicSelectedPartitionPtr.GetRawPointer();
}

