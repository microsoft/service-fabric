// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionHealthStateFilter");

PartitionHealthStateFilter::PartitionHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , partitionIdFilter_(Guid::Empty())
    , replicaFilters_()
{
}

PartitionHealthStateFilter::PartitionHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , partitionIdFilter_(Guid::Empty())
    , replicaFilters_()
{
}

PartitionHealthStateFilter::~PartitionHealthStateFilter()
{
}

wstring PartitionHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<PartitionHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode PartitionHealthStateFilter::FromJsonString(wstring const & str, __inout PartitionHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool PartitionHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = false;
    if (partitionIdFilter_ != Guid::Empty() || !replicaFilters_.empty())
    {
        // If this is a specific filter or has children filter specified, consider default as all
        returnAllOnDefault = true;
    }

    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode PartitionHealthStateFilter::FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_FILTER const & publicPartitionHealthStateFilter)
{
    healthStateFilter_ = publicPartitionHealthStateFilter.HealthStateFilter;

    partitionIdFilter_ = Guid(publicPartitionHealthStateFilter.PartitionIdFilter);

    // ReplicaFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<ReplicaHealthStateFilter, FABRIC_REPLICA_HEALTH_STATE_FILTER_LIST>(publicPartitionHealthStateFilter.ReplicaFilters, replicaFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ReplicaFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode PartitionHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_PARTITION_HEALTH_STATE_FILTER & publicFilter) const
{
    ErrorCode error(ErrorCodeValue::Success);

    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.PartitionIdFilter = partitionIdFilter_.AsGUID();

    auto publicReplicaFilters = heap.AddItem<FABRIC_REPLICA_HEALTH_STATE_FILTER_LIST>();

    error = PublicApiHelper::ToPublicApiList<ReplicaHealthStateFilter, FABRIC_REPLICA_HEALTH_STATE_FILTER, FABRIC_REPLICA_HEALTH_STATE_FILTER_LIST>(
        heap,
        replicaFilters_,
        *publicReplicaFilters);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "ReplicaFilters::ToPublicApi error: {0}", error);
        return error;
    }

    publicFilter.ReplicaFilters = publicReplicaFilters.GetRawPointer();
    return ErrorCode::Success();
}
