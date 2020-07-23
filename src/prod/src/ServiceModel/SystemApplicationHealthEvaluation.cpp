// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("SystemAppEvaluation");

INITIALIZE_SIZE_ESTIMATION(SystemApplicationHealthEvaluation)

SystemApplicationHealthEvaluation::SystemApplicationHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION)
{
}

SystemApplicationHealthEvaluation::SystemApplicationHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION, aggregatedHealthState, move(unhealthyEvaluations))
{
}

ErrorCode SystemApplicationHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicSystemApplicationHealthEvaluation = heap.AddItem<FABRIC_SYSTEM_APPLICATION_HEALTH_EVALUATION>();

    publicSystemApplicationHealthEvaluation->Description = heap.AddString(description_);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicSystemApplicationHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicSystemApplicationHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;
    
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION;
    publicHealthEvaluation.Value = publicSystemApplicationHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode SystemApplicationHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicSystemApplicationHealthEvaluation = reinterpret_cast<FABRIC_SYSTEM_APPLICATION_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);
    auto hr = StringUtility::LpcwstrToWstring(publicSystemApplicationHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicSystemApplicationHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicSystemApplicationHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void SystemApplicationHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthySystemApplication,
        aggregatedHealthState_);
}
