// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(ApplicationHealth)

StringLiteral const TraceSource("ApplicationHealth");

ApplicationHealth::ApplicationHealth()
    : EntityHealthBase()
    , applicationName_()
    , servicesAggregatedHealthStates_()
    , deployedApplicationsAggregatedHealthStates_()
{
}

ApplicationHealth::ApplicationHealth(
    std::wstring const & applicationName,        
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    std::vector<ServiceAggregatedHealthState> && servicesAggregatedHealthStates,
    std::vector<DeployedApplicationAggregatedHealthState> && deployedApplicationsAggregatedHealthStates,
    HealthStatisticsUPtr && healthStatistics)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), move(healthStatistics))
    , applicationName_(applicationName)
    , servicesAggregatedHealthStates_(move(servicesAggregatedHealthStates))
    , deployedApplicationsAggregatedHealthStates_(move(deployedApplicationsAggregatedHealthStates))
{
}

ApplicationHealth::~ApplicationHealth()
{
}

Common::ErrorCode ApplicationHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_APPLICATION_HEALTH & publicApplicationHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicApplicationHealth.ApplicationName = heap.AddString(applicationName_);

    // Health state
    publicApplicationHealth.AggregatedHealthState = aggregatedHealthState_;

    // Services
    auto publicServicesList = heap.AddItem<FABRIC_SERVICE_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<ServiceAggregatedHealthState, FABRIC_SERVICE_HEALTH_STATE, FABRIC_SERVICE_HEALTH_STATE_LIST>(
        heap,
        servicesAggregatedHealthStates_,
        *publicServicesList);
    if (!error.IsSuccess()) { return error; }
    publicApplicationHealth.ServiceHealthStates = publicServicesList.GetRawPointer();

    // DeployedApplications
    auto publicDeployedApplicationsList = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<DeployedApplicationAggregatedHealthState, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_LIST>(
        heap, 
        deployedApplicationsAggregatedHealthStates_, 
        *publicDeployedApplicationsList);
    if (!error.IsSuccess()) { return error; }
    publicApplicationHealth.DeployedApplicationHealthStates = publicDeployedApplicationsList.GetRawPointer();

    // Events 
    auto publicHealthEventList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicHealthEventList);
    if (!error.IsSuccess()) { return error; }
    publicApplicationHealth.HealthEvents = publicHealthEventList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    auto ex1 = heap.AddItem<FABRIC_APPLICATION_HEALTH_EX1>();
    ex1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    // Health Statistics
    if (healthStats_)
    {
        auto ex2 = heap.AddItem<FABRIC_APPLICATION_HEALTH_EX2>();
        auto publicHealthStats = heap.AddItem<FABRIC_HEALTH_STATISTICS>();
        error = healthStats_->ToPublicApi(heap, *publicHealthStats);
        if (!error.IsSuccess()) { return error; }

        ex2->HealthStatistics = publicHealthStats.GetRawPointer();

        ex1->Reserved = ex2.GetRawPointer();
    }

    publicApplicationHealth.Reserved = ex1.GetRawPointer();
    
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationHealth::FromPublicApi(
    FABRIC_APPLICATION_HEALTH const & publicApplicationHealth)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicApplicationHealth.ApplicationName, true /* acceptNull */, applicationName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }

    // Health state
    aggregatedHealthState_ = publicApplicationHealth.AggregatedHealthState;
        
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicApplicationHealth.HealthEvents, events_);
    if (!error.IsSuccess()) { return error; }

    // Services
    error = PublicApiHelper::FromPublicApiList<ServiceAggregatedHealthState, FABRIC_SERVICE_HEALTH_STATE_LIST>(publicApplicationHealth.ServiceHealthStates, servicesAggregatedHealthStates_);
    if (!error.IsSuccess()) { return error; }

    // DeployedApplications
    error = PublicApiHelper::FromPublicApiList<DeployedApplicationAggregatedHealthState, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_LIST>(publicApplicationHealth.DeployedApplicationHealthStates, deployedApplicationsAggregatedHealthStates_);
    if (!error.IsSuccess()) { return error; }

    if (publicApplicationHealth.Reserved != NULL)
    {
        auto ex1 = static_cast<FABRIC_APPLICATION_HEALTH_EX1*>(publicApplicationHealth.Reserved);

        // Evaluation reasons
        error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
            ex1->UnhealthyEvaluations,
            unhealthyEvaluations_);
        if (!error.IsSuccess()) { return error; }

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

void ApplicationHealth::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write(
            "{0}, {1} events, {2} services, {3} deployed applications, stats [{4}].",
            aggregatedHealthState_,
            events_.size(),
            servicesAggregatedHealthStates_.size(),
            deployedApplicationsAggregatedHealthStates_.size(),
            this->GetStatsString());
    }
    else
    {
        writer.Write(
            "{0}, {1} events, {2} services, {3} deployed applications, stats [{4}], UnhealthyEvaluations: {5}",
            aggregatedHealthState_,
            events_.size(),
            servicesAggregatedHealthStates_.size(),
            deployedApplicationsAggregatedHealthStates_.size(),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

void ApplicationHealth::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModelEventSource::Trace->ApplicationHealthTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(servicesAggregatedHealthStates_.size()),
            static_cast<uint64>(deployedApplicationsAggregatedHealthStates_.size()),
            this->GetStatsString());
    }
    else
    {
        ServiceModelEventSource::Trace->ApplicationHealthWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(servicesAggregatedHealthStates_.size()),
            static_cast<uint64>(deployedApplicationsAggregatedHealthStates_.size()),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}
