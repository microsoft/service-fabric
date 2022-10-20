// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("AppsEvaluation");

INITIALIZE_SIZE_ESTIMATION(ApplicationsHealthEvaluation)

ApplicationsHealthEvaluation::ApplicationsHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS)
    , maxPercentUnhealthyApplications_(0)
    , totalCount_(0)
{
}

ApplicationsHealthEvaluation::ApplicationsHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyApplications)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS, aggregatedHealthState, move(unhealthyEvaluations))
    , maxPercentUnhealthyApplications_(maxPercentUnhealthyApplications)
    , totalCount_(totalCount)
{
}

ApplicationsHealthEvaluation::~ApplicationsHealthEvaluation()
{
}

ErrorCode ApplicationsHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicApplicationsHealthEvaluation = heap.AddItem<FABRIC_APPLICATIONS_HEALTH_EVALUATION>();

    publicApplicationsHealthEvaluation->Description = heap.AddString(description_);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicApplicationsHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicApplicationsHealthEvaluation->MaxPercentUnhealthyApplications = maxPercentUnhealthyApplications_;

    publicApplicationsHealthEvaluation->TotalCount = totalCount_;

    publicApplicationsHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS;
    publicHealthEvaluation.Value = publicApplicationsHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationsHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicApplicationsHealthEvaluation = reinterpret_cast<FABRIC_APPLICATIONS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicApplicationsHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyApplications_ = publicApplicationsHealthEvaluation->MaxPercentUnhealthyApplications;

    totalCount_ = publicApplicationsHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicApplicationsHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicApplicationsHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ApplicationsHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyApplicationsPerPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        maxPercentUnhealthyApplications_);
}
