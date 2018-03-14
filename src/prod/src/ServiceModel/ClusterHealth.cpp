// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(ClusterHealth)

StringLiteral const TraceSource("ClusterHealth");

ClusterHealth::ClusterHealth()
    : EntityHealthBase()
    , nodesAggregatedHealthStates_()
    , applicationsAggregatedHealthStates_()
{
}

ClusterHealth::ClusterHealth(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    vector<NodeAggregatedHealthState> && nodesAggregatedHealthStates,
    vector<ApplicationAggregatedHealthState> && applicationsAggregatedHealthStates,
    vector<HealthEvent> && events,
    HealthStatisticsUPtr && healthStatistics)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), move(healthStatistics))
    , nodesAggregatedHealthStates_(move(nodesAggregatedHealthStates))
    , applicationsAggregatedHealthStates_(move(applicationsAggregatedHealthStates))
{
}

ClusterHealth::~ClusterHealth()
{
}

Common::ErrorCode ClusterHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_CLUSTER_HEALTH & publicClusterHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    publicClusterHealth.AggregatedHealthState = aggregatedHealthState_;

    auto ex1 = heap.AddItem<FABRIC_CLUSTER_HEALTH_EX1>();

    // Nodes
    auto publicNodesList = heap.AddItem<FABRIC_NODE_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<NodeAggregatedHealthState, FABRIC_NODE_HEALTH_STATE, FABRIC_NODE_HEALTH_STATE_LIST>(
        heap,
        nodesAggregatedHealthStates_,
        *publicNodesList);
    if (!error.IsSuccess()) { return error; }
    ex1->NodeHealthStates = publicNodesList.GetRawPointer();

    // Applications
    auto publicApplicationsList = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<ApplicationAggregatedHealthState, FABRIC_APPLICATION_HEALTH_STATE, FABRIC_APPLICATION_HEALTH_STATE_LIST>(
        heap,
        applicationsAggregatedHealthStates_,
        *publicApplicationsList);
    if (!error.IsSuccess()) { return error; }
    ex1->ApplicationHealthStates = publicApplicationsList.GetRawPointer();

    // Events
    auto publicEventsList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicEventsList);
    if (!error.IsSuccess()) { return error; }
    ex1->HealthEvents = publicEventsList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto ex2 = heap.AddItem<FABRIC_CLUSTER_HEALTH_EX2>();
    ex2->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    // Health Statistics
    if (healthStats_)
    {
        auto ex3 = heap.AddItem<FABRIC_CLUSTER_HEALTH_EX3>();
        auto publicHealthStats = heap.AddItem<FABRIC_HEALTH_STATISTICS>();
        error = healthStats_->ToPublicApi(heap, *publicHealthStats);
        if (!error.IsSuccess()) { return error; }

        ex3->HealthStatistics = publicHealthStats.GetRawPointer();

        ex2->Reserved = ex3.GetRawPointer();
    }

    ex1->Reserved = ex2.GetRawPointer();
    
    publicClusterHealth.Reserved = ex1.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealth::FromPublicApi(
    FABRIC_CLUSTER_HEALTH const & publicClusterHealth)
{
    ErrorCode error(ErrorCodeValue::Success);
    aggregatedHealthState_ = publicClusterHealth.AggregatedHealthState;

    if (publicClusterHealth.Reserved != NULL)
    {
        FABRIC_CLUSTER_HEALTH_EX1 * ex1 = (FABRIC_CLUSTER_HEALTH_EX1*) publicClusterHealth.Reserved;

        // Nodes
        error = PublicApiHelper::FromPublicApiList<NodeAggregatedHealthState, FABRIC_NODE_HEALTH_STATE_LIST>(
            ex1->NodeHealthStates,
            nodesAggregatedHealthStates_);
        if (!error.IsSuccess()) { return error; }

        // Applications
        error = PublicApiHelper::FromPublicApiList<ApplicationAggregatedHealthState, FABRIC_APPLICATION_HEALTH_STATE_LIST>(
            ex1->ApplicationHealthStates,
            applicationsAggregatedHealthStates_);
        if (!error.IsSuccess()) { return error; }

        // Events
        error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(
            ex1->HealthEvents, 
            events_);
        if (!error.IsSuccess()) { return error; }

        if (ex1->Reserved != NULL)
        {
            auto ex2 = (FABRIC_CLUSTER_HEALTH_EX2*)ex1->Reserved;
    
            // Evaluation reasons
            error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
                ex2->UnhealthyEvaluations,
                unhealthyEvaluations_);
            if (!error.IsSuccess()) { return error; }

            if (ex2->Reserved != nullptr)
            {
                auto ex3 = (FABRIC_CLUSTER_HEALTH_EX3*)ex2->Reserved;
                // HealthStatistics
                if (ex3->HealthStatistics != nullptr)
                {
                    healthStats_ = make_unique<HealthStatistics>();
                    error = healthStats_->FromPublicApi(ex3->HealthStatistics);
                    if (!error.IsSuccess()) { return error; }
                }
            }
        }
    }

    return ErrorCode::Success();
}

void ClusterHealth::WriteTo(TextWriter & w, FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        w.Write("{0}, {1} events, {2} applications, {3} nodes, stats [{4}].",
            aggregatedHealthState_,
            events_.size(),
            applicationsAggregatedHealthStates_.size(),
            nodesAggregatedHealthStates_.size(),
            this->GetStatsString());
    }
    else
    {
        w.Write("{0}, {1} events, {2} applications, {3} nodes, stats [{4}]. UnhealthyEvaluations: {5}",
            aggregatedHealthState_,
            events_.size(),
            applicationsAggregatedHealthStates_.size(),
            nodesAggregatedHealthStates_.size(),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

void ClusterHealth::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModelEventSource::Trace->ClusterHealthTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(applicationsAggregatedHealthStates_.size()),
            static_cast<uint64>(nodesAggregatedHealthStates_.size()),
            this->GetStatsString());
    }
    else
    {
        ServiceModelEventSource::Trace->ClusterHealthWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(applicationsAggregatedHealthStates_.size()),
            static_cast<uint64>(nodesAggregatedHealthStates_.size()),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

wstring ClusterHealth::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterHealth&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ClusterHealth::FromString(wstring const & clusterHealthStr, __out ClusterHealth & clusterHealth)
{
    return JsonHelper::Deserialize(clusterHealth, clusterHealthStr);
}
