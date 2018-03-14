// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationHealth)

StringLiteral const TraceSource("DeployedApplicationHealth");

DeployedApplicationHealth::DeployedApplicationHealth()
    : EntityHealthBase()
    , applicationName_()
    , nodeName_()
    , deployedServicePackagesAggregatedHealthStates_()
{
}

DeployedApplicationHealth::DeployedApplicationHealth(
    std::wstring const & applicationName,        
    std::wstring const & nodeName,
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    std::vector<DeployedServicePackageAggregatedHealthState> && deployedServicePackagesAggregatedHealthStates,
    HealthStatisticsUPtr && healthStatistics)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), move(healthStatistics))
    , applicationName_(applicationName)
    , nodeName_(nodeName)
    , deployedServicePackagesAggregatedHealthStates_(move(deployedServicePackagesAggregatedHealthStates))
{
}

DeployedApplicationHealth::~DeployedApplicationHealth()
{
}

Common::ErrorCode DeployedApplicationHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_APPLICATION_HEALTH & publicDeployedApplicationHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicDeployedApplicationHealth.ApplicationName = heap.AddString(applicationName_);
    publicDeployedApplicationHealth.NodeName = heap.AddString(nodeName_);

    // Health state
    publicDeployedApplicationHealth.AggregatedHealthState = aggregatedHealthState_;

    // Events
    auto publicEventsList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicEventsList);
    if (!error.IsSuccess()) { return error; }
    publicDeployedApplicationHealth.HealthEvents = publicEventsList.GetRawPointer();

    // Deployed service packages
    auto publicDeployedServicePackagesList = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_LIST>();
    error = PublicApiHelper::ToPublicApiList<DeployedServicePackageAggregatedHealthState, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_LIST>(
        heap, 
        deployedServicePackagesAggregatedHealthStates_, 
        *publicDeployedServicePackagesList);
    if (!error.IsSuccess()) { return error; }
    publicDeployedApplicationHealth.DeployedServicePackageHealthStates = publicDeployedServicePackagesList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto ex1 = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_EX1>();
    ex1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
       
    // Health Statistics
    if (healthStats_)
    {
        auto ex2 = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_EX2>();
        auto publicHealthStats = heap.AddItem<FABRIC_HEALTH_STATISTICS>();
        error = healthStats_->ToPublicApi(heap, *publicHealthStats);
        if (!error.IsSuccess()) { return error; }
        ex2->HealthStatistics = publicHealthStats.GetRawPointer();

        ex1->Reserved = ex2.GetRawPointer();
    }

    publicDeployedApplicationHealth.Reserved = ex1.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationHealth::FromPublicApi(
    FABRIC_DEPLOYED_APPLICATION_HEALTH const & publicDeployedApplicationHealth)
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealth.ApplicationName, false, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealth.NodeName, false, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    // Health state
    aggregatedHealthState_ = publicDeployedApplicationHealth.AggregatedHealthState;
        
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicDeployedApplicationHealth.HealthEvents, events_);
    if (!error.IsSuccess()) { return error; }

    // Deployed service packages
    error = PublicApiHelper::FromPublicApiList<DeployedServicePackageAggregatedHealthState, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_LIST>(publicDeployedApplicationHealth.DeployedServicePackageHealthStates, deployedServicePackagesAggregatedHealthStates_);
    if (!error.IsSuccess()) { return error; }
    
    if (publicDeployedApplicationHealth.Reserved != NULL)
    {
        auto ex1 = (FABRIC_DEPLOYED_APPLICATION_HEALTH_EX1*) publicDeployedApplicationHealth.Reserved;
    
        // Evaluation reasons
        error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
            ex1->UnhealthyEvaluations,
            unhealthyEvaluations_);
        if (!error.IsSuccess()) { return error; }

        if (ex1->Reserved != nullptr)
        {
            auto ex2 = (FABRIC_DEPLOYED_APPLICATION_HEALTH_EX2*)ex1->Reserved;
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

void DeployedApplicationHealth::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write(
            "{0}, {1} events, {2} deployed service packages, stats [{3}].",
            aggregatedHealthState_,
            events_.size(),
            deployedServicePackagesAggregatedHealthStates_.size(),
            this->GetStatsString());
    }
    else
    {
        writer.Write(
            "{0}, {1} events, {2} deployed service packages, stats [{3}], UnhealthyEvaluations: {4}",
            aggregatedHealthState_,
            events_.size(),
            deployedServicePackagesAggregatedHealthStates_.size(),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

void DeployedApplicationHealth::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModelEventSource::Trace->DeployedApplicationHealthTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(deployedServicePackagesAggregatedHealthStates_.size()),
            this->GetStatsString());
    }
    else
    {
        ServiceModelEventSource::Trace->DeployedApplicationHealthWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            static_cast<uint64>(deployedServicePackagesAggregatedHealthStates_.size()),
            this->GetStatsString(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}
