// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(ServiceHealth)

StringLiteral const TraceSource("ServiceHealth");

ServiceHealth::ServiceHealth()
    : EntityHealthBase()
    , serviceName_()
    , partitionsAggregatedHealthStates_()
{
}

ServiceHealth::ServiceHealth(
    std::wstring const & serviceName,        
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    std::vector<PartitionAggregatedHealthState> && partitionsAggregatedHealthStates,
    HealthStatisticsUPtr && healthStatistics)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), move(healthStatistics))
    , serviceName_(serviceName)
    , partitionsAggregatedHealthStates_(move(partitionsAggregatedHealthStates))
{
}

ServiceHealth::~ServiceHealth()
{
}

Common::ErrorCode ServiceHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_HEALTH & publicServiceHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicServiceHealth.ServiceName = heap.AddString(serviceName_);

    // Health state
    publicServiceHealth.AggregatedHealthState = aggregatedHealthState_;

    // Events
    auto publicHealthEventList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicHealthEventList);
    if (!error.IsSuccess()) { return error; }
    publicServiceHealth.HealthEvents = publicHealthEventList.GetRawPointer();

    // Partitions
    auto publicPartitionsList = heap.AddItem<FABRIC_PARTITION_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<PartitionAggregatedHealthState, FABRIC_PARTITION_HEALTH_STATE, FABRIC_PARTITION_HEALTH_STATE_LIST>(
        heap, 
        partitionsAggregatedHealthStates_, 
        *publicPartitionsList);
    if (!error.IsSuccess()) { return error; }
    publicServiceHealth.PartitionHealthStates = publicPartitionsList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto ex1 = heap.AddItem<FABRIC_SERVICE_HEALTH_EX1>();
    ex1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    // Health Statistics
    if (healthStats_)
    {
        auto ex2 = heap.AddItem<FABRIC_SERVICE_HEALTH_EX2>();
        auto publicHealthStats = heap.AddItem<FABRIC_HEALTH_STATISTICS>();
        error = healthStats_->ToPublicApi(heap, *publicHealthStats);
        if (!error.IsSuccess()) { return error; }
        ex2->HealthStatistics = publicHealthStats.GetRawPointer();

        ex1->Reserved = ex2.GetRawPointer();
    }

    publicServiceHealth.Reserved = ex1.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode ServiceHealth::FromPublicApi(
    FABRIC_SERVICE_HEALTH const & publicServiceHealth)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicServiceHealth.ServiceName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing service name in FromPublicAPI: {0}", error);
        return error;
    }

    // Health state
    aggregatedHealthState_ = publicServiceHealth.AggregatedHealthState;
        
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicServiceHealth.HealthEvents, events_);
    if (!error.IsSuccess()) 
    {
        Trace.WriteInfo(TraceSource, "Error parsing health events in FromPublicAPI: {0}", error);
        return error; 
    }
    
    // Partitions
    error = PublicApiHelper::FromPublicApiList<PartitionAggregatedHealthState, FABRIC_PARTITION_HEALTH_STATE_LIST>(
        publicServiceHealth.PartitionHealthStates,
        partitionsAggregatedHealthStates_);
    if (!error.IsSuccess()) 
    {
        Trace.WriteInfo(TraceSource, "Error parsing partition aggregated health states in FromPublicAPI: {0}", error);
        return error; 
    }
    
    if (publicServiceHealth.Reserved != NULL)
    {
        FABRIC_SERVICE_HEALTH_EX1 * ex1 = (FABRIC_SERVICE_HEALTH_EX1*) publicServiceHealth.Reserved;

        // Evaluation reasons
        error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
            ex1->UnhealthyEvaluations,
            unhealthyEvaluations_);
        if (!error.IsSuccess()) 
        {
            Trace.WriteInfo(TraceSource, "Error parsing health evaluations in FromPublicAPI: {0}", error);
            return error; 
        }

        if (ex1->Reserved != nullptr)
        {
            auto ex2 = (FABRIC_SERVICE_HEALTH_EX2*)ex1->Reserved;
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

void ServiceHealth::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write(
            "{0}, {1} events, {2} partitions, stats [{3}].",
            aggregatedHealthState_,
            events_.size(),
            partitionsAggregatedHealthStates_.size(),
            this->GetStatsString());
    }
    else
    {
        writer.Write(
            "{0}, {1} events, {2} partitions, stats [{3}], UnhealthyEvaluations: {4}",
            aggregatedHealthState_,
            events_.size(),
            partitionsAggregatedHealthStates_.size(),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

void ServiceHealth::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModelEventSource::Trace->ServiceHealthTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(partitionsAggregatedHealthStates_.size()),
            this->GetStatsString());
    }
    else
    {
        ServiceModelEventSource::Trace->ServiceHealthWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(partitionsAggregatedHealthStates_.size()),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}
