// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceEvaluation");

INITIALIZE_SIZE_ESTIMATION(ServiceHealthEvaluation)

ServiceHealthEvaluation::ServiceHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SERVICE)
    , serviceName_()
{
}

ServiceHealthEvaluation::ServiceHealthEvaluation(
    std::wstring const & serviceName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SERVICE, aggregatedHealthState, move(unhealthyEvaluations))
    , serviceName_(serviceName)
{
}

ServiceHealthEvaluation::~ServiceHealthEvaluation()
{
}

ErrorCode ServiceHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicServiceHealthEvaluation = heap.AddItem<FABRIC_SERVICE_HEALTH_EVALUATION>();

    publicServiceHealthEvaluation->Description = heap.AddString(description_);
    publicServiceHealthEvaluation->ServiceName = heap.AddString(serviceName_);
    publicServiceHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicServiceHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_SERVICE;
    publicHealthEvaluation.Value = publicServiceHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ServiceHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicServiceHealthEvaluation = reinterpret_cast<FABRIC_SERVICE_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicServiceHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicServiceHealthEvaluation->ServiceName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing service name in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicServiceHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicServiceHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ServiceHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyService,
        serviceName_,
        aggregatedHealthState_);
}
