// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ReplicasEvaluation");

INITIALIZE_SIZE_ESTIMATION(ReplicasHealthEvaluation)

ReplicasHealthEvaluation::ReplicasHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_REPLICAS)
    , maxPercentUnhealthyReplicasPerPartition_(0)
    , totalCount_(0)
{
}

ReplicasHealthEvaluation::ReplicasHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyReplicasPerPartition)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_REPLICAS, aggregatedHealthState, move(unhealthyEvaluations))
    , maxPercentUnhealthyReplicasPerPartition_(maxPercentUnhealthyReplicasPerPartition)
    , totalCount_(totalCount)
{
}

ReplicasHealthEvaluation::~ReplicasHealthEvaluation()
{
}

ErrorCode ReplicasHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicReplicasHealthEvaluation = heap.AddItem<FABRIC_REPLICAS_HEALTH_EVALUATION>();

    publicReplicasHealthEvaluation->Description = heap.AddString(description_);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicReplicasHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicReplicasHealthEvaluation->MaxPercentUnhealthyReplicasPerPartition = maxPercentUnhealthyReplicasPerPartition_;

    publicReplicasHealthEvaluation->TotalCount = totalCount_;

    publicReplicasHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_REPLICAS;
    publicHealthEvaluation.Value = publicReplicasHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ReplicasHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicReplicasHealthEvaluation = reinterpret_cast<FABRIC_REPLICAS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicReplicasHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyReplicasPerPartition_ = publicReplicasHealthEvaluation->MaxPercentUnhealthyReplicasPerPartition;

    totalCount_ = publicReplicasHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicReplicasHealthEvaluation->AggregatedHealthState;

    // Replicas
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicReplicasHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ReplicasHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyReplicasPerPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        maxPercentUnhealthyReplicasPerPartition_);
}
