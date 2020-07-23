// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionEvaluation");

INITIALIZE_SIZE_ESTIMATION(PartitionHealthEvaluation)

PartitionHealthEvaluation::PartitionHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_PARTITION)
    , partitionId_(Guid::Empty())
{
}

PartitionHealthEvaluation::PartitionHealthEvaluation(
    Common::Guid const & partitionId,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_PARTITION, aggregatedHealthState, move(unhealthyEvaluations))
    , partitionId_(partitionId)
{
}

PartitionHealthEvaluation::~PartitionHealthEvaluation()
{
}

ErrorCode PartitionHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicPartitionHealthEvaluation = heap.AddItem<FABRIC_PARTITION_HEALTH_EVALUATION>();

    publicPartitionHealthEvaluation->Description = heap.AddString(description_);
    publicPartitionHealthEvaluation->PartitionId = partitionId_.AsGUID();
    publicPartitionHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicPartitionHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_PARTITION;
    publicHealthEvaluation.Value = publicPartitionHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode PartitionHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicPartitionHealthEvaluation = reinterpret_cast<FABRIC_PARTITION_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    partitionId_ = Guid(publicPartitionHealthEvaluation->PartitionId);

    auto hr = StringUtility::LpcwstrToWstring(publicPartitionHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicPartitionHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicPartitionHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void PartitionHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyPartition,
        partitionId_,
        aggregatedHealthState_);
}
