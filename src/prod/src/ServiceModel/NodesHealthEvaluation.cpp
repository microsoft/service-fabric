// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodesEvaluation");

INITIALIZE_SIZE_ESTIMATION(NodesHealthEvaluation)

NodesHealthEvaluation::NodesHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_NODES)
    , maxPercentUnhealthyNodes_(0)
    , totalCount_(0)
{
}

NodesHealthEvaluation::NodesHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyNodes)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_NODES, aggregatedHealthState, move(unhealthyEvaluations))
    , maxPercentUnhealthyNodes_(maxPercentUnhealthyNodes)
    , totalCount_(totalCount)
{
}

NodesHealthEvaluation::~NodesHealthEvaluation()
{
}

ErrorCode NodesHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicNodesHealthEvaluation = heap.AddItem<FABRIC_NODES_HEALTH_EVALUATION>();

    publicNodesHealthEvaluation->Description = heap.AddString(description_);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicNodesHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicNodesHealthEvaluation->MaxPercentUnhealthyNodes = maxPercentUnhealthyNodes_;

    publicNodesHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicNodesHealthEvaluation->TotalCount = totalCount_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_NODES;
    publicHealthEvaluation.Value = publicNodesHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode NodesHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicNodesHealthEvaluation = reinterpret_cast<FABRIC_NODES_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicNodesHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyNodes_ = publicNodesHealthEvaluation->MaxPercentUnhealthyNodes;

    totalCount_ = publicNodesHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicNodesHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicNodesHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void NodesHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyNodesPerPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        maxPercentUnhealthyNodes_);
}
