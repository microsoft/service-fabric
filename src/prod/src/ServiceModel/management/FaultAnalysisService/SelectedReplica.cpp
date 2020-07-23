// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("SelectedPartition");

SelectedReplica::SelectedReplica()
: replicaOrInstanceId_(FABRIC_INVALID_REPLICA_ID)
, selectedPartition_()
{
}

SelectedReplica::SelectedReplica(SelectedReplica const & other)
: replicaOrInstanceId_(other.replicaOrInstanceId_)
, selectedPartition_(other.selectedPartition_)
{
}

bool SelectedReplica::operator == (SelectedReplica const & other) const
{
    return (replicaOrInstanceId_ == other.ReplicaOrInstanceId)
        && (selectedPartition_ == other.Partition);
}

bool SelectedReplica::operator != (SelectedReplica const & other) const
{
    return !(*this == other);
}

SelectedReplica & SelectedReplica::operator = (SelectedReplica const & other)
{
    if (this != &other)
    {
        replicaOrInstanceId_ = other.replicaOrInstanceId_;
        selectedPartition_ = other.selectedPartition_;
    }

    return *this;
}

SelectedReplica::SelectedReplica(SelectedReplica && other)
: replicaOrInstanceId_(move(other.replicaOrInstanceId_))
, selectedPartition_(move(other.selectedPartition_))
{
}

SelectedReplica & SelectedReplica::operator = (SelectedReplica && other)
{
    if (this != &other)
    {
        replicaOrInstanceId_ = move(other.replicaOrInstanceId_);
        selectedPartition_ = move(other.selectedPartition_);
    }

    return *this;
}

Common::ErrorCode SelectedReplica::FromPublicApi(
    FABRIC_SELECTED_REPLICA const & publicSelectedReplica)
{
    replicaOrInstanceId_ = publicSelectedReplica.ReplicaOrInstanceId;

    SelectedPartition selectedPartition;
    auto error = selectedPartition.FromPublicApi(*publicSelectedReplica.SelectedPartition);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "SelectedReplica::FromPublicApi/Failed at result.FromPublicApi");
        return error;
    }

    selectedPartition_ = make_shared<SelectedPartition>(selectedPartition);

    return ErrorCode::Success();
}

void SelectedReplica::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SELECTED_REPLICA & result) const
{
    result.ReplicaOrInstanceId = replicaOrInstanceId_;

    if (selectedPartition_)
    {
        auto resultPtr = heap.AddItem<FABRIC_SELECTED_PARTITION>();

        selectedPartition_->ToPublicApi(heap, *resultPtr);

        result.SelectedPartition = resultPtr.GetRawPointer();
    }
}
