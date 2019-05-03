// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeEvaluation");

INITIALIZE_SIZE_ESTIMATION(NodeHealthEvaluation)

NodeHealthEvaluation::NodeHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_NODE)
    , nodeName_()
{
}

NodeHealthEvaluation::NodeHealthEvaluation(
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_NODE, aggregatedHealthState, move(unhealthyEvaluations))
    , nodeName_(nodeName)
{
}

NodeHealthEvaluation::~NodeHealthEvaluation()
{
}

ErrorCode NodeHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicNodeHealthEvaluation = heap.AddItem<FABRIC_NODE_HEALTH_EVALUATION>();

    publicNodeHealthEvaluation->Description = heap.AddString(description_);
    publicNodeHealthEvaluation->NodeName = heap.AddString(nodeName_);
    publicNodeHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicNodeHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_NODE;
    publicHealthEvaluation.Value = publicNodeHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode NodeHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicNodeHealthEvaluation = reinterpret_cast<FABRIC_NODE_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicNodeHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicNodeHealthEvaluation->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicNodeHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicNodeHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void NodeHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyNode,
        nodeName_,
        aggregatedHealthState_);
}
