// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedAppsEvaluation");

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationsHealthEvaluation)

DeployedApplicationsHealthEvaluation::DeployedApplicationsHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS)
    , maxPercentUnhealthyDeployedApplications_(0)
    , totalCount_(0)
{
}

DeployedApplicationsHealthEvaluation::DeployedApplicationsHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyDeployedApplications)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS, aggregatedHealthState, move(unhealthyEvaluations))
    , maxPercentUnhealthyDeployedApplications_(maxPercentUnhealthyDeployedApplications)
    , totalCount_(totalCount)
{
}

DeployedApplicationsHealthEvaluation::~DeployedApplicationsHealthEvaluation()
{
}

ErrorCode DeployedApplicationsHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicDeployedApplicationsHealthEvaluation = heap.AddItem<FABRIC_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION>();

    publicDeployedApplicationsHealthEvaluation->Description = heap.AddString(description_);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicDeployedApplicationsHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicDeployedApplicationsHealthEvaluation->MaxPercentUnhealthyDeployedApplications = maxPercentUnhealthyDeployedApplications_;

    publicDeployedApplicationsHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicDeployedApplicationsHealthEvaluation->TotalCount = totalCount_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS;
    publicHealthEvaluation.Value = publicDeployedApplicationsHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationsHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicDeployedApplicationsHealthEvaluation = reinterpret_cast<FABRIC_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationsHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyDeployedApplications_ = publicDeployedApplicationsHealthEvaluation->MaxPercentUnhealthyDeployedApplications;
    
    totalCount_ = publicDeployedApplicationsHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicDeployedApplicationsHealthEvaluation->AggregatedHealthState;

    // DeployedApplications
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicDeployedApplicationsHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void DeployedApplicationsHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeployedApplicationsPerPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        maxPercentUnhealthyDeployedApplications_);
}
