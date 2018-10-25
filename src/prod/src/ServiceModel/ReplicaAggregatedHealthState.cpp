// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ReplicaAggregatedHealthState)

StringLiteral const TraceSource("ReplicaAggregatedHealthState");

ReplicaAggregatedHealthState::ReplicaAggregatedHealthState()
    : kind_(FABRIC_SERVICE_KIND_INVALID)
    , partitionId_(Guid::Empty())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

ReplicaAggregatedHealthState::ReplicaAggregatedHealthState(
    FABRIC_SERVICE_KIND kind,
    Common::Guid const & partitionId,        
    FABRIC_REPLICA_ID replicaId,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : kind_(kind)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

ReplicaAggregatedHealthState::~ReplicaAggregatedHealthState()
{
}

Common::ErrorCode ReplicaAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REPLICA_HEALTH_STATE & publicReplicaHealth) const 
{
    switch (kind_)
    {
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            auto replica = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_STATE>();
            replica->ReplicaId = replicaId_;
            replica->PartitionId = partitionId_.AsGUID();
            replica->AggregatedHealthState = aggregatedHealthState_;

            publicReplicaHealth.Value = replica.GetRawPointer();
            break;
        }
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            auto instance = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_STATE>();
            instance->InstanceId = replicaId_;
            instance->PartitionId = partitionId_.AsGUID();
            instance->AggregatedHealthState = aggregatedHealthState_;

            publicReplicaHealth.Value = instance.GetRawPointer();
            break;
        }
    default:
        Trace.WriteInfo(TraceSource, "ToPublicApi: kind {0} is not supported", static_cast<int>(kind_));
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    publicReplicaHealth.Kind = kind_;
    return ErrorCode::Success();
}

Common::ErrorCode ReplicaAggregatedHealthState::FromPublicApi(
    FABRIC_REPLICA_HEALTH_STATE const & publicReplicaHealth)
{
    switch (publicReplicaHealth.Kind)
    {
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            auto replica = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH*>(publicReplicaHealth.Value);

            partitionId_ = Guid(replica->PartitionId);
            replicaId_= replica->ReplicaId;
            aggregatedHealthState_ = replica->AggregatedHealthState;
            break;
        }
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            auto instance = reinterpret_cast<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH*>(publicReplicaHealth.Value);

            partitionId_ = Guid(instance->PartitionId);
            replicaId_= instance->InstanceId;
            aggregatedHealthState_ = instance->AggregatedHealthState;
            break;
        }
    default:
        Trace.WriteInfo(TraceSource, "FromPublicApi: kind {0} is not supported", static_cast<int>(publicReplicaHealth.Kind));
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void ReplicaAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ReplicaAggregatedHealthState({0}+{1}: {2})", partitionId_, replicaId_, aggregatedHealthState_);
}

std::wstring ReplicaAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring ReplicaAggregatedHealthState::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<FABRIC_REPLICA_ID>(replicaId_);
}
