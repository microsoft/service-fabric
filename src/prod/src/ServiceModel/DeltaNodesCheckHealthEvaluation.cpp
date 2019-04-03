// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeltaNodesCheckHealthEvaluation");

INITIALIZE_SIZE_ESTIMATION(DeltaNodesCheckHealthEvaluation)

DeltaNodesCheckHealthEvaluation::DeltaNodesCheckHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK)
    , baselineErrorCount_(0)
    , baselineTotalCount_(0)
    , totalCount_(0)
    , maxPercentDeltaUnhealthyNodes_(0)
{
}

DeltaNodesCheckHealthEvaluation::DeltaNodesCheckHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    ULONG baselineErrorCount,
    ULONG baselineTotalCount,
    ULONG totalCount,
    BYTE maxPercentDeltaUnhealthyNodes,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK, aggregatedHealthState, move(unhealthyEvaluations))
    , baselineErrorCount_(baselineErrorCount)
    , baselineTotalCount_(baselineTotalCount)
    , totalCount_(totalCount)
    , maxPercentDeltaUnhealthyNodes_(maxPercentDeltaUnhealthyNodes)
{
}

DeltaNodesCheckHealthEvaluation::~DeltaNodesCheckHealthEvaluation()
{
}

ErrorCode DeltaNodesCheckHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    auto publicDeltaNodesCheckHealthEvaluation = reinterpret_cast<FABRIC_DELTA_NODES_CHECK_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    // Description
    {
        auto hr = StringUtility::LpcwstrToWstring(publicDeltaNodesCheckHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr);
            Trace.WriteInfo(TraceSource, "Description::FromPublicApi error: {0}", error);
            return error;
        }
    }

    aggregatedHealthState_ = publicDeltaNodesCheckHealthEvaluation->AggregatedHealthState;
    baselineErrorCount_ = publicDeltaNodesCheckHealthEvaluation->BaselineErrorCount;
    baselineTotalCount_ = publicDeltaNodesCheckHealthEvaluation->BaselineTotalCount;
    totalCount_ = publicDeltaNodesCheckHealthEvaluation->TotalCount;
    maxPercentDeltaUnhealthyNodes_ = publicDeltaNodesCheckHealthEvaluation->MaxPercentDeltaUnhealthyNodes;

    // UnhealthyEvaluations
    {
        auto error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(publicDeltaNodesCheckHealthEvaluation->UnhealthyEvaluations, unhealthyEvaluations_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "UnhealthyEvaluations::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

ErrorCode DeltaNodesCheckHealthEvaluation::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicDeltaNodesCheckHealthEvaluation = heap.AddItem<FABRIC_DELTA_NODES_CHECK_HEALTH_EVALUATION>();

    publicDeltaNodesCheckHealthEvaluation->Description = heap.AddString(description_);
    publicDeltaNodesCheckHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;
    publicDeltaNodesCheckHealthEvaluation->BaselineErrorCount = baselineErrorCount_;
    publicDeltaNodesCheckHealthEvaluation->BaselineTotalCount = baselineTotalCount_;
    publicDeltaNodesCheckHealthEvaluation->TotalCount = totalCount_;
    publicDeltaNodesCheckHealthEvaluation->MaxPercentDeltaUnhealthyNodes = maxPercentDeltaUnhealthyNodes_;

    // UnhealthyEvaluations
    {
        auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
        auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(heap, unhealthyEvaluations_, *publicHealthEvaluationList);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "UnhealthyEvaluations::ToPublicApi error: {0}", error);
            return error;
        }
        publicDeltaNodesCheckHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    }

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK;
    publicHealthEvaluation.Value = publicDeltaNodesCheckHealthEvaluation.GetRawPointer();

    return ErrorCode::Success();
}

void DeltaNodesCheckHealthEvaluation::SetDescription()
{
    size_t currentUnhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeltaNodesCheck,
        GetUnhealthyPercent(currentUnhealthyCount, totalCount_),
        currentUnhealthyCount,
        totalCount_,
        GetUnhealthyPercent(baselineErrorCount_, baselineTotalCount_),
        baselineErrorCount_,
        baselineTotalCount_,
        maxPercentDeltaUnhealthyNodes_);
}
