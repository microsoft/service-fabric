// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackagesEvaluation");

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackagesHealthEvaluation)

DeployedServicePackagesHealthEvaluation::DeployedServicePackagesHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES)
    , totalCount_(0)
{
}

DeployedServicePackagesHealthEvaluation::DeployedServicePackagesHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES, aggregatedHealthState, move(unhealthyEvaluations))
    , totalCount_(totalCount)
{
}

DeployedServicePackagesHealthEvaluation::~DeployedServicePackagesHealthEvaluation()
{
}

ErrorCode DeployedServicePackagesHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicDeployedServicePackagesHealthEvaluation = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGES_HEALTH_EVALUATION>();

    publicDeployedServicePackagesHealthEvaluation->Description = heap.AddString(description_);

    publicDeployedServicePackagesHealthEvaluation->TotalCount = totalCount_;

    publicDeployedServicePackagesHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicUnhealthyEvaluations);
    publicDeployedServicePackagesHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES;
    publicHealthEvaluation.Value = publicDeployedServicePackagesHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackagesHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicDeployedServicePackagesHealthEvaluation = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_PACKAGES_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    totalCount_ = publicDeployedServicePackagesHealthEvaluation->TotalCount;
    auto hr = StringUtility::LpcwstrToWstring(publicDeployedServicePackagesHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicDeployedServicePackagesHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicDeployedServicePackagesHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void DeployedServicePackagesHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeployedServicePackages,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_);
}
