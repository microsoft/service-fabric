// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationEvaluation");

INITIALIZE_SIZE_ESTIMATION(ApplicationHealthEvaluation)

ApplicationHealthEvaluation::ApplicationHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION)
    , applicationName_()
{
}

ApplicationHealthEvaluation::ApplicationHealthEvaluation(
    std::wstring const & applicationName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION, aggregatedHealthState, move(unhealthyEvaluations))
    , applicationName_(applicationName)
{
}

ApplicationHealthEvaluation::~ApplicationHealthEvaluation()
{
}

ErrorCode ApplicationHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicApplicationHealthEvaluation = heap.AddItem<FABRIC_APPLICATION_HEALTH_EVALUATION>();

    publicApplicationHealthEvaluation->Description = heap.AddString(description_);
    publicApplicationHealthEvaluation->ApplicationName = heap.AddString(applicationName_);
    publicApplicationHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicApplicationHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_APPLICATION;
    publicHealthEvaluation.Value = publicApplicationHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicApplicationHealthEvaluation = reinterpret_cast<FABRIC_APPLICATION_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicApplicationHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    // Application name is only empty for adhoc
    hr = StringUtility::LpcwstrToWstring(publicApplicationHealthEvaluation->ApplicationName, true, 0, ParameterValidator::MaxStringSize, applicationName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicApplicationHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicApplicationHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ApplicationHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyApplication,
        applicationName_,
        aggregatedHealthState_);
}
