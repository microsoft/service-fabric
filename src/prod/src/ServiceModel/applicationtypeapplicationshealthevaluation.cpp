// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("AppTypeAppsEvaluation");

INITIALIZE_SIZE_ESTIMATION(ApplicationTypeApplicationsHealthEvaluation)

ApplicationTypeApplicationsHealthEvaluation::ApplicationTypeApplicationsHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS)
    , maxPercentUnhealthyApplications_(0)
    , totalCount_(0)
{
}

ApplicationTypeApplicationsHealthEvaluation::ApplicationTypeApplicationsHealthEvaluation(
    std::wstring const & applicationTypeName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyApplications)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS, aggregatedHealthState, move(unhealthyEvaluations))
    , applicationTypeName_(applicationTypeName)
    , maxPercentUnhealthyApplications_(maxPercentUnhealthyApplications)
    , totalCount_(totalCount)
{
}

ApplicationTypeApplicationsHealthEvaluation::~ApplicationTypeApplicationsHealthEvaluation()
{
}

ErrorCode ApplicationTypeApplicationsHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicAppTypeAppsHealthEvaluation = heap.AddItem<FABRIC_APPLICATION_TYPE_APPLICATIONS_HEALTH_EVALUATION>();

    publicAppTypeAppsHealthEvaluation->Description = heap.AddString(description_);

    publicAppTypeAppsHealthEvaluation->ApplicationTypeName = heap.AddString(applicationTypeName_);

    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicAppTypeAppsHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicAppTypeAppsHealthEvaluation->MaxPercentUnhealthyApplications = maxPercentUnhealthyApplications_;

    publicAppTypeAppsHealthEvaluation->TotalCount = totalCount_;

    publicAppTypeAppsHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS;
    publicHealthEvaluation.Value = publicAppTypeAppsHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationTypeApplicationsHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicAppTypeAppsHealthEvaluation = reinterpret_cast<FABRIC_APPLICATION_TYPE_APPLICATIONS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicAppTypeAppsHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicAppTypeAppsHealthEvaluation->ApplicationTypeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, applicationTypeName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing applicationTypeName in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyApplications_ = publicAppTypeAppsHealthEvaluation->MaxPercentUnhealthyApplications;

    totalCount_ = publicAppTypeAppsHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicAppTypeAppsHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicAppTypeAppsHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ApplicationTypeApplicationsHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyApplicationTypeApplications,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        applicationTypeName_,
        maxPercentUnhealthyApplications_);
}
