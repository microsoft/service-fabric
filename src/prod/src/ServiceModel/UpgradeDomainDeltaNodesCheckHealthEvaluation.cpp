// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("UpgradeDomainDeltaNodesCheckHealthEvaluation");

INITIALIZE_SIZE_ESTIMATION(UpgradeDomainDeltaNodesCheckHealthEvaluation)

UpgradeDomainDeltaNodesCheckHealthEvaluation::UpgradeDomainDeltaNodesCheckHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK)
    , upgradeDomainName_()
    , baselineErrorCount_(0)
    , baselineTotalCount_(0)
    , totalCount_(0)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(0)
{
}

UpgradeDomainDeltaNodesCheckHealthEvaluation::UpgradeDomainDeltaNodesCheckHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::wstring const & upgradeDomainName,
    ULONG baselineErrorCount,
    ULONG baselineTotalCount,
    ULONG totalCount,
    BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK, aggregatedHealthState, move(unhealthyEvaluations))
    , upgradeDomainName_(upgradeDomainName)
    , baselineErrorCount_(baselineErrorCount)
    , baselineTotalCount_(baselineTotalCount)
    , totalCount_(totalCount)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(maxPercentUpgradeDomainDeltaUnhealthyNodes)
{
}

UpgradeDomainDeltaNodesCheckHealthEvaluation::~UpgradeDomainDeltaNodesCheckHealthEvaluation()
{
}

ErrorCode UpgradeDomainDeltaNodesCheckHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    auto publicUDDeltaNodesCheckHealthEvaluation = reinterpret_cast<FABRIC_UPGRADE_DOMAIN_DELTA_NODES_CHECK_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    // Description
    {
        auto hr = StringUtility::LpcwstrToWstring(publicUDDeltaNodesCheckHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr);
            Trace.WriteInfo(TraceSource, "Description::FromPublicApi error: {0}", error);
            return error;
        }
    }

    aggregatedHealthState_ = publicUDDeltaNodesCheckHealthEvaluation->AggregatedHealthState;

    // UpgradeDomainName
    {
        auto hr = StringUtility::LpcwstrToWstring(publicUDDeltaNodesCheckHealthEvaluation->UpgradeDomainName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, upgradeDomainName_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr);
            Trace.WriteInfo(TraceSource, "UpgradeDomainName::FromPublicApi error: {0}", error);
            return error;
        }
    }

    baselineErrorCount_ = publicUDDeltaNodesCheckHealthEvaluation->BaselineErrorCount;
    baselineTotalCount_ = publicUDDeltaNodesCheckHealthEvaluation->BaselineTotalCount;
    totalCount_ = publicUDDeltaNodesCheckHealthEvaluation->TotalCount;
    maxPercentUpgradeDomainDeltaUnhealthyNodes_ = publicUDDeltaNodesCheckHealthEvaluation->MaxPercentUpgradeDomainDeltaUnhealthyNodes;

    // UnhealthyEvaluations
    {
        auto error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(publicUDDeltaNodesCheckHealthEvaluation->UnhealthyEvaluations, unhealthyEvaluations_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "UnhealthyEvaluations::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

ErrorCode UpgradeDomainDeltaNodesCheckHealthEvaluation::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicUDDeltaNodesCheckHealthEvaluation = heap.AddItem<FABRIC_UPGRADE_DOMAIN_DELTA_NODES_CHECK_HEALTH_EVALUATION>();

    publicUDDeltaNodesCheckHealthEvaluation->Description = heap.AddString(description_);

    publicUDDeltaNodesCheckHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicUDDeltaNodesCheckHealthEvaluation->UpgradeDomainName = heap.AddString(upgradeDomainName_);

    publicUDDeltaNodesCheckHealthEvaluation->BaselineErrorCount = baselineErrorCount_;
    publicUDDeltaNodesCheckHealthEvaluation->BaselineTotalCount = baselineTotalCount_;
    publicUDDeltaNodesCheckHealthEvaluation->TotalCount = totalCount_;
    publicUDDeltaNodesCheckHealthEvaluation->MaxPercentUpgradeDomainDeltaUnhealthyNodes = maxPercentUpgradeDomainDeltaUnhealthyNodes_;

    // UnhealthyEvaluations
    {
        auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
        auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(heap, unhealthyEvaluations_, *publicHealthEvaluationList);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "UnhealthyEvaluations::ToPublicApi error: {0}", error);
            return error;
        }
        publicUDDeltaNodesCheckHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    }

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK;
    publicHealthEvaluation.Value = publicUDDeltaNodesCheckHealthEvaluation.GetRawPointer();

    return ErrorCode::Success();
}

void UpgradeDomainDeltaNodesCheckHealthEvaluation::SetDescription()
{
    size_t currentUnhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyUpgradeDomainDeltaNodesCheck,
        upgradeDomainName_,
        GetUnhealthyPercent(currentUnhealthyCount, totalCount_),
        currentUnhealthyCount,
        totalCount_,
        GetUnhealthyPercent(baselineErrorCount_, baselineTotalCount_),
        baselineErrorCount_,
        baselineTotalCount_,
        maxPercentUpgradeDomainDeltaUnhealthyNodes_);
}
