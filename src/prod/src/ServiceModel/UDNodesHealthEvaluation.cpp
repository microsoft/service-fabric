// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodesPerUdHealthEvaluationReason");

INITIALIZE_SIZE_ESTIMATION(UDNodesHealthEvaluation)

UDNodesHealthEvaluation::UDNodesHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES)
    , upgradeDomainName_()
    , maxPercentUnhealthyNodes_(0)
    , totalCount_(0)
{
}

UDNodesHealthEvaluation::UDNodesHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::wstring const & upgradeDomainName,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyNodes)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES, aggregatedHealthState, move(unhealthyEvaluations))
    , upgradeDomainName_(upgradeDomainName)
    , maxPercentUnhealthyNodes_(maxPercentUnhealthyNodes)
    , totalCount_(totalCount)
{
}

ErrorCode UDNodesHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicUDNodesHealthEvaluation = heap.AddItem<FABRIC_UPGRADE_DOMAIN_NODES_HEALTH_EVALUATION>();

    publicUDNodesHealthEvaluation->UpgradeDomainName = heap.AddString(upgradeDomainName_);

    publicUDNodesHealthEvaluation->Description = heap.AddString(description_);

    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicUnhealthyEvaluations);
    publicUDNodesHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicUDNodesHealthEvaluation->MaxPercentUnhealthyNodes = maxPercentUnhealthyNodes_;

    publicUDNodesHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicUDNodesHealthEvaluation->TotalCount = totalCount_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES;
    publicHealthEvaluation.Value = publicUDNodesHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode UDNodesHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicUDNodesHealthEvaluation = reinterpret_cast<FABRIC_UPGRADE_DOMAIN_NODES_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicUDNodesHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicUDNodesHealthEvaluation->UpgradeDomainName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, upgradeDomainName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing upgrade domain name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    maxPercentUnhealthyNodes_ = publicUDNodesHealthEvaluation->MaxPercentUnhealthyNodes;

    totalCount_ = publicUDNodesHealthEvaluation->TotalCount;

    aggregatedHealthState_= publicUDNodesHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicUDNodesHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void UDNodesHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyNodesPerUDPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        upgradeDomainName_,
        maxPercentUnhealthyNodes_);
}
