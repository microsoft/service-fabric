// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ReplicaHealth)

StringLiteral const TraceSource("ReplicaHealth");

ReplicaHealth::ReplicaHealth()
    : EntityHealthBase()
    , kind_(FABRIC_SERVICE_KIND_INVALID)
    , partitionId_(Guid::Empty())
    , replicaId_(FABRIC_INVALID_REPLICA_ID)
{
}

ReplicaHealth::ReplicaHealth(
    FABRIC_SERVICE_KIND kind,
    Common::Guid const & partitionId,        
    FABRIC_REPLICA_ID replicaId,
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), nullptr)
    , kind_(kind)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
{
}

ReplicaHealth::~ReplicaHealth()
{
}

Common::ErrorCode ReplicaHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REPLICA_HEALTH & publicReplicaHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Events
    auto publicEventsList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicEventsList);
    if (!error.IsSuccess()) { return error; }

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    switch (kind_)
    {
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            auto replica = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH>();
            replica->ReplicaId = replicaId_;
            replica->PartitionId = partitionId_.AsGUID();
            replica->AggregatedHealthState = aggregatedHealthState_;
            replica->HealthEvents = publicEventsList.GetRawPointer();

            auto publicReplicaHealthEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_EX1>();
            publicReplicaHealthEx1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
            replica->Reserved = publicReplicaHealthEx1.GetRawPointer();

            publicReplicaHealth.Value = replica.GetRawPointer();
            break;
        }
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            auto instance = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH>();
            instance->InstanceId = replicaId_;
            instance->PartitionId = partitionId_.AsGUID();
            instance->AggregatedHealthState = aggregatedHealthState_;
            instance->HealthEvents = publicEventsList.GetRawPointer();

            auto publicInstanceHealthEx1 = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_EX1>();
            publicInstanceHealthEx1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
            instance->Reserved = publicInstanceHealthEx1.GetRawPointer();

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

Common::ErrorCode ReplicaHealth::FromPublicApi(
    FABRIC_REPLICA_HEALTH const & publicReplicaHealth)
{
    ErrorCode error(ErrorCodeValue::Success);
    FABRIC_HEALTH_EVENT_LIST publicEvents = {0};
    
    switch (publicReplicaHealth.Kind)
    {
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            auto replica = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH*>(publicReplicaHealth.Value);

            partitionId_ = Guid(replica->PartitionId);
            replicaId_= replica->ReplicaId;
            aggregatedHealthState_ = replica->AggregatedHealthState;
            publicEvents = *replica->HealthEvents;

            if (replica->Reserved != NULL)
            {
                auto replicaHealthEx1 = (FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_EX1*) replica->Reserved;
    
                // Evaluation reasons
                error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
                    replicaHealthEx1->UnhealthyEvaluations,
                    unhealthyEvaluations_);
                if (!error.IsSuccess()) { return error; }
            }
            break;
        }
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            auto instance = reinterpret_cast<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH*>(publicReplicaHealth.Value);

            partitionId_ = Guid(instance->PartitionId);
            replicaId_= instance->InstanceId;
            aggregatedHealthState_ = instance->AggregatedHealthState;
            publicEvents = *instance->HealthEvents;

            if (instance->Reserved != NULL)
            {
                auto instanceHealthEx1 = (FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_EX1*) instance->Reserved;
    
                // Evaluation reasons
                error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
                    instanceHealthEx1->UnhealthyEvaluations,
                    unhealthyEvaluations_);
                if (!error.IsSuccess()) { return error; }
            }
            break;
        }
    default:
        Trace.WriteInfo(TraceSource, "FromPublicApi: kind {0} is not supported", static_cast<int>(publicReplicaHealth.Kind));
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(&publicEvents, events_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
