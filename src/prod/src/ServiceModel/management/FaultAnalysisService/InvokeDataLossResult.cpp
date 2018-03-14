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

StringLiteral const TraceComponent("InvokeDataLossResult");

InvokeDataLossResult::InvokeDataLossResult() 
    : errorCode_(0)
    , selectedPartition_()
{ 
}

InvokeDataLossResult::InvokeDataLossResult(InvokeDataLossResult && other)
    : errorCode_(move(other.errorCode_))
    , selectedPartition_(move(other.selectedPartition_))
{ 
}

ErrorCode InvokeDataLossResult::FromPublicApi(FABRIC_PARTITION_DATA_LOSS_RESULT const & publicResult)
{
    errorCode_ = publicResult.ErrorCode;

    SelectedPartition selectedPartition;

    FABRIC_SELECTED_PARTITION * publicSelectedPartition = publicResult.SelectedPartition;

    auto error = selectedPartition.FromPublicApi(*publicSelectedPartition);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "InvokeDataLossDescription::FromPublicApi/Failed at selectedPartition.FromPublicApi");
        return error;
    }

    selectedPartition_ = make_shared<SelectedPartition>(selectedPartition);
    
    return ErrorCodeValue::Success;
}

void InvokeDataLossResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_PARTITION_DATA_LOSS_RESULT & publicResult) const
{   
    auto publicSelectedPartitionPtr = heap.AddItem<FABRIC_SELECTED_PARTITION>();
    selectedPartition_->ToPublicApi(heap, *publicSelectedPartitionPtr);
    publicResult.SelectedPartition = publicSelectedPartitionPtr.GetRawPointer();

    publicResult.ErrorCode = errorCode_;
}

