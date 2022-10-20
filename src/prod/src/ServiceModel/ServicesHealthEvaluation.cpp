// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServicesEvaluation");

INITIALIZE_SIZE_ESTIMATION(ServicesHealthEvaluation)

ServicesHealthEvaluation::ServicesHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SERVICES)
    , serviceTypeName_()
    , maxPercentUnhealthyServices_(0)
    , totalCount_(0)
{
}

ServicesHealthEvaluation::ServicesHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::wstring const & serviceTypeName,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyServices)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_SERVICES, aggregatedHealthState, move(unhealthyEvaluations))
    , serviceTypeName_(serviceTypeName)
    , maxPercentUnhealthyServices_(maxPercentUnhealthyServices)
    , totalCount_(totalCount)
{
}

ServicesHealthEvaluation::~ServicesHealthEvaluation()
{
}

ErrorCode ServicesHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicServicesHealthEvaluation = heap.AddItem<FABRIC_SERVICES_HEALTH_EVALUATION>();

    publicServicesHealthEvaluation->ServiceTypeName = heap.AddString(serviceTypeName_);

    publicServicesHealthEvaluation->Description = heap.AddString(description_);

    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap,
        unhealthyEvaluations_,
        *publicUnhealthyEvaluations);
    publicServicesHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicServicesHealthEvaluation->MaxPercentUnhealthyServices = maxPercentUnhealthyServices_;
    publicServicesHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;
    publicServicesHealthEvaluation->TotalCount = totalCount_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_SERVICES;
    publicHealthEvaluation.Value = publicServicesHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode ServicesHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicServicesHealthEvaluation = reinterpret_cast<FABRIC_SERVICES_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicServicesHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicServicesHealthEvaluation->ServiceTypeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceTypeName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing service type name in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyServices_ = publicServicesHealthEvaluation->MaxPercentUnhealthyServices;

    totalCount_ = publicServicesHealthEvaluation->TotalCount;

    aggregatedHealthState_ = publicServicesHealthEvaluation->AggregatedHealthState;

    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicServicesHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void ServicesHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyServicesPerServiceTypePolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        serviceTypeName_,
        maxPercentUnhealthyServices_);
}
