// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(PartitionHealth)

StringLiteral const TraceSource("PartitionHealth");

PartitionHealth::PartitionHealth()
    : EntityHealthBase()
    , partitionId_(Guid::Empty())
    , replicasAggregatedHealthStates_()
{
}

PartitionHealth::PartitionHealth(
    Common::Guid const & partitionId,        
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    std::vector<ReplicaAggregatedHealthState> && replicasAggregatedHealthStates,
    HealthStatisticsUPtr && healthStatistics)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), move(healthStatistics))
    , partitionId_(partitionId)
    , replicasAggregatedHealthStates_(move(replicasAggregatedHealthStates))
{
}

PartitionHealth::~PartitionHealth()
{
}

Common::ErrorCode PartitionHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_PARTITION_HEALTH & publicPartitionHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicPartitionHealth.PartitionId = partitionId_.AsGUID();

    // Health state
    publicPartitionHealth.AggregatedHealthState = aggregatedHealthState_;

    // Events
    auto publicHealthEventList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicHealthEventList);
    if (!error.IsSuccess()) { return error; }
    publicPartitionHealth.HealthEvents = publicHealthEventList.GetRawPointer();

    // Replicas
    auto publicReplicasList = heap.AddItem<FABRIC_REPLICA_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<ReplicaAggregatedHealthState, FABRIC_REPLICA_HEALTH_STATE, FABRIC_REPLICA_HEALTH_STATE_LIST>(
        heap, 
        replicasAggregatedHealthStates_, 
        *publicReplicasList);
    if (!error.IsSuccess()) { return error; }
    publicPartitionHealth.ReplicaHealthStates = publicReplicasList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto ex1 = heap.AddItem<FABRIC_PARTITION_HEALTH_EX1>();
    ex1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    // Health Statistics
    if (healthStats_)
    {
        auto ex2 = heap.AddItem<FABRIC_PARTITION_HEALTH_EX2>();
        auto publicHealthStats = heap.AddItem<FABRIC_HEALTH_STATISTICS>();
        error = healthStats_->ToPublicApi(heap, *publicHealthStats);
        if (!error.IsSuccess()) { return error; }
        ex2->HealthStatistics = publicHealthStats.GetRawPointer();

        ex1->Reserved = ex2.GetRawPointer();
    }

    publicPartitionHealth.Reserved = ex1.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode PartitionHealth::FromPublicApi(
    FABRIC_PARTITION_HEALTH const & publicPartitionHealth)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    partitionId_ = Guid(publicPartitionHealth.PartitionId);

    // Health state
    aggregatedHealthState_ = publicPartitionHealth.AggregatedHealthState;
        
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicPartitionHealth.HealthEvents, events_);
    if (!error.IsSuccess()) { return error; }

    // Replicas
    error = PublicApiHelper::FromPublicApiList<ReplicaAggregatedHealthState, FABRIC_REPLICA_HEALTH_STATE_LIST>(
        publicPartitionHealth.ReplicaHealthStates,
        replicasAggregatedHealthStates_);
    if (!error.IsSuccess()) { return error; }
    
    if (publicPartitionHealth.Reserved != NULL)
    {
        auto ex1 = (FABRIC_PARTITION_HEALTH_EX1*) publicPartitionHealth.Reserved;

        // Evaluation reasons
        error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
            ex1->UnhealthyEvaluations,
            unhealthyEvaluations_);
        if (!error.IsSuccess()) { return error; }

        if (ex1->Reserved != nullptr)
        {
            auto ex2 = (FABRIC_PARTITION_HEALTH_EX2*)ex1->Reserved;
            // HealthStatistics
            if (ex2->HealthStatistics != nullptr)
            {
                healthStats_ = make_unique<HealthStatistics>();
                error = healthStats_->FromPublicApi(ex2->HealthStatistics);
                if (!error.IsSuccess()) { return error; }
            }
        }
    }

    return ErrorCode::Success();
}

void PartitionHealth::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write(
            "{0}, {1} events, {2} replicas, stats [{3}].",
            aggregatedHealthState_,
            events_.size(),
            replicasAggregatedHealthStates_.size(),
            this->GetStatsString());
    }
    else
    {
        writer.Write(
            "{0}, {1} events, {2} replicas, stats [{3}]. UnhealthyEvaluations: {4}",
            aggregatedHealthState_,
            events_.size(),
            replicasAggregatedHealthStates_.size(),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

void PartitionHealth::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModelEventSource::Trace->PartitionHealthTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(replicasAggregatedHealthStates_.size()),
            this->GetStatsString());
    }
    else
    {
        ServiceModelEventSource::Trace->PartitionHealthWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(replicasAggregatedHealthStates_.size()),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}
