// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationEvaluation");

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationHealthEvaluation)

DeployedApplicationHealthEvaluation::DeployedApplicationHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION)
    , nodeName_()
    , applicationName_()
{
}

DeployedApplicationHealthEvaluation::DeployedApplicationHealthEvaluation(
    std::wstring const & applicationName,  
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION, aggregatedHealthState, move(unhealthyEvaluations))
    , nodeName_(nodeName)
    , applicationName_(applicationName)
{
}

DeployedApplicationHealthEvaluation::~DeployedApplicationHealthEvaluation()
{
}

ErrorCode DeployedApplicationHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicDeployedApplicationHealthEvaluation = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_EVALUATION>();

    publicDeployedApplicationHealthEvaluation->Description = heap.AddString(description_);
    publicDeployedApplicationHealthEvaluation->ApplicationName = heap.AddString(applicationName_);
    publicDeployedApplicationHealthEvaluation->NodeName = heap.AddString(nodeName_);
    publicDeployedApplicationHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }
    publicDeployedApplicationHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION;
    publicHealthEvaluation.Value = publicDeployedApplicationHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicDeployedApplicationHealthEvaluation = reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthEvaluation->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthEvaluation->ApplicationName, true, 0, ParameterValidator::MaxStringSize, applicationName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicDeployedApplicationHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicDeployedApplicationHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void DeployedApplicationHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeployedApplication,
        nodeName_,
        aggregatedHealthState_);
}
