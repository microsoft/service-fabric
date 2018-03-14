// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(NodeHealth)

StringLiteral const TraceSource("NodeHealth");

NodeHealth::NodeHealth()
    : EntityHealthBase()
    , nodeName_()
{
}

NodeHealth::NodeHealth(
    std::wstring const & nodeName,
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations), nullptr)
    , nodeName_(nodeName)
{
}

NodeHealth::~NodeHealth()
{
}

Common::ErrorCode NodeHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_NODE_HEALTH & publicNodeHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicNodeHealth.NodeName = heap.AddString(nodeName_);

    // Health state
    publicNodeHealth.AggregatedHealthState = aggregatedHealthState_;
    
    // Events
    auto publicHealthEventList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicHealthEventList);
    if (!error.IsSuccess()) { return error; }
    publicNodeHealth.HealthEvents = publicHealthEventList.GetRawPointer();

    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto publicNodeHealthEx1 = heap.AddItem<FABRIC_NODE_HEALTH_EX1>();
    publicNodeHealthEx1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    publicNodeHealth.Reserved = publicNodeHealthEx1.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode NodeHealth::FromPublicApi(
    FABRIC_NODE_HEALTH const & publicNodeHealth)
{
    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicNodeHealth.NodeName, true, 0, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    ErrorCode error(ErrorCodeValue::Success);
    
    // Health state
    aggregatedHealthState_ = publicNodeHealth.AggregatedHealthState;
        
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicNodeHealth.HealthEvents, events_);
    if (!error.IsSuccess()) { return error; }
    
    if (publicNodeHealth.Reserved != NULL)
    {
        auto nodeHealthEx1 = (FABRIC_NODE_HEALTH_EX1*) publicNodeHealth.Reserved;
    
        // Evaluation reasons
        error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
            nodeHealthEx1->UnhealthyEvaluations,
            unhealthyEvaluations_);
        if (!error.IsSuccess()) { return error; }
    }
    
    return ErrorCode::Success();
}
