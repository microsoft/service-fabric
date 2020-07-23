// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackageEvaluation");

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageHealthEvaluation)

DeployedServicePackageHealthEvaluation::DeployedServicePackageHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE)
    , nodeName_()
    , applicationName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
{
}

DeployedServicePackageHealthEvaluation::DeployedServicePackageHealthEvaluation(
    std::wstring const & applicationName,  
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE, aggregatedHealthState, move(unhealthyEvaluations))
    , nodeName_(nodeName)
    , applicationName_(applicationName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
{
}

DeployedServicePackageHealthEvaluation::~DeployedServicePackageHealthEvaluation()
{
}

ErrorCode DeployedServicePackageHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicDeployedServicePackageHealthEvaluation = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION>();

    publicDeployedServicePackageHealthEvaluation->Description = heap.AddString(description_);
    publicDeployedServicePackageHealthEvaluation->ApplicationName = heap.AddString(applicationName_);
    publicDeployedServicePackageHealthEvaluation->NodeName = heap.AddString(nodeName_);
    publicDeployedServicePackageHealthEvaluation->ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedServicePackageHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;
    
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);

    if (!error.IsSuccess()) 
    { 
        return error; 
    }

    publicDeployedServicePackageHealthEvaluation->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    
    auto healthEvaluationEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION_EX1>();

    healthEvaluationEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    healthEvaluationEx1->Reserved = nullptr;
    publicDeployedServicePackageHealthEvaluation->Reserved = healthEvaluationEx1.GetRawPointer();

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE;
    publicHealthEvaluation.Value = publicDeployedServicePackageHealthEvaluation.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackageHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    auto publicDeployedServicePackageHealthEvaluation = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
   if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

   error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthEvaluation->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
   if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

   error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthEvaluation->ApplicationName, true, 0, ParameterValidator::MaxStringSize, applicationName_);
   if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }

   error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthEvaluation->ServiceManifestName, false, serviceManifestName_);
   if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing service manifest name in FromPublicAPI: {0}", error);
        return error;
    }

    aggregatedHealthState_ = publicDeployedServicePackageHealthEvaluation->AggregatedHealthState;
    
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicDeployedServicePackageHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    if (publicDeployedServicePackageHealthEvaluation->Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto queryResultEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION_EX1*)publicDeployedServicePackageHealthEvaluation->Reserved;

   error = StringUtility::LpcwstrToWstring2(queryResultEx1->ServicePackageActivationId, true, servicePackageActivationId_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing ServicePackageActivationId in FromPublicAPI: {0}", error);
        return error;
    }

    return error;
}

void DeployedServicePackageHealthEvaluation::SetDescription()
{
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeployedServicePackage,
        serviceManifestName_,
        servicePackageActivationId_,
        aggregatedHealthState_);
}
