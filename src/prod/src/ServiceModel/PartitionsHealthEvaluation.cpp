// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionsEvaluation");

INITIALIZE_SIZE_ESTIMATION(PartitionsHealthEvaluation)

PartitionsHealthEvaluation::PartitionsHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS)
    , maxPercentUnhealthyPartitionsPerService_(0)
    , totalCount_(0)
{
}

PartitionsHealthEvaluation::PartitionsHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyPartitionsPerService)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS, aggregatedHealthState, move(unhealthyEvaluations))
    , maxPercentUnhealthyPartitionsPerService_(maxPercentUnhealthyPartitionsPerService)
    , totalCount_(totalCount)
{
}

PartitionsHealthEvaluation::~PartitionsHealthEvaluation()
{
}

ErrorCode PartitionsHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicPartitionsHealthEvaluation = heap.AddItem<FABRIC_PARTITIONS_HEALTH_EVALUATION>();

    publicPartitionsHealthEvaluation->Description = heap.AddString(description_);

    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicUnhealthyEvaluations);
    publicPartitionsHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicPartitionsHealthEvaluation->TotalCount = totalCount_;
    publicPartitionsHealthEvaluation->MaxPercentUnhealthyPartitionsPerService = maxPercentUnhealthyPartitionsPerService_;
    publicPartitionsHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS;
    publicHealthEvaluation.Value = publicPartitionsHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode PartitionsHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicPartitionsHealthEvaluation = reinterpret_cast<FABRIC_PARTITIONS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicPartitionsHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyPartitionsPerService_ = publicPartitionsHealthEvaluation->MaxPercentUnhealthyPartitionsPerService;
    totalCount_ = publicPartitionsHealthEvaluation->TotalCount;
    aggregatedHealthState_ = publicPartitionsHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicPartitionsHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void PartitionsHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyPartitionsPerPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        maxPercentUnhealthyPartitionsPerService_);
}
