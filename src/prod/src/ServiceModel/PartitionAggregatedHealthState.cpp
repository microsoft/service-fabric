// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(PartitionAggregatedHealthState)

StringLiteral const TraceSource("PartitionAggregatedHealthState");

PartitionAggregatedHealthState::PartitionAggregatedHealthState()
    : partitionId_(Guid::Empty())
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

PartitionAggregatedHealthState::PartitionAggregatedHealthState(
    Common::Guid const & partitionId,        
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : partitionId_(partitionId)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

PartitionAggregatedHealthState::~PartitionAggregatedHealthState()
{
}

Common::ErrorCode PartitionAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_PARTITION_HEALTH_STATE & publicPartitionAggregatedHealthState) const 
{
    UNREFERENCED_PARAMETER(heap);

    // Entity information
    publicPartitionAggregatedHealthState.PartitionId = partitionId_.AsGUID();

    // Health state
    publicPartitionAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;

    return ErrorCode::Success();
}

Common::ErrorCode PartitionAggregatedHealthState::FromPublicApi(
    FABRIC_PARTITION_HEALTH_STATE const & publicPartitionAggregatedHealthState)
{
    // Entity information
    partitionId_ = Guid(publicPartitionAggregatedHealthState.PartitionId);

    // Health state
    aggregatedHealthState_ = publicPartitionAggregatedHealthState.AggregatedHealthState;
   
    return ErrorCode::Success();
}

void PartitionAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("PartitionAggregatedHealthState({0}: {1})", partitionId_, aggregatedHealthState_);
}

std::wstring PartitionAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring PartitionAggregatedHealthState::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Guid>(partitionId_);
}
