// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedAppsPerUdEvaluation");

INITIALIZE_SIZE_ESTIMATION(UDDeployedApplicationsHealthEvaluation)

UDDeployedApplicationsHealthEvaluation::UDDeployedApplicationsHealthEvaluation()
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS)
    , upgradeDomainName_()
    , maxPercentUnhealthyDeployedApplications_(0)
    , totalCount_(0)
{
}

UDDeployedApplicationsHealthEvaluation::UDDeployedApplicationsHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::wstring const & upgradeDomainName,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    ULONG totalCount,
    BYTE maxPercentUnhealthyDeployedApplications)
    : HealthEvaluationWithChildrenBase(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS, aggregatedHealthState, move(unhealthyEvaluations))
    , upgradeDomainName_(upgradeDomainName)
    , maxPercentUnhealthyDeployedApplications_(maxPercentUnhealthyDeployedApplications)
    , totalCount_(totalCount)
{
}

ErrorCode UDDeployedApplicationsHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicUDDeployedApplicationsHealthEvaluation = heap.AddItem<FABRIC_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION>();

    publicUDDeployedApplicationsHealthEvaluation->UpgradeDomainName = heap.AddString(upgradeDomainName_);

    publicUDDeployedApplicationsHealthEvaluation->Description = heap.AddString(this->Description);
    
    auto publicUnhealthyEvaluations = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicUnhealthyEvaluations);
    publicUDDeployedApplicationsHealthEvaluation->UnhealthyEvaluations = publicUnhealthyEvaluations.GetRawPointer();

    publicUDDeployedApplicationsHealthEvaluation->MaxPercentUnhealthyDeployedApplications = maxPercentUnhealthyDeployedApplications_;

    publicUDDeployedApplicationsHealthEvaluation->TotalCount = totalCount_;

    publicUDDeployedApplicationsHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS;
    publicHealthEvaluation.Value = publicUDDeployedApplicationsHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode UDDeployedApplicationsHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicUDDeployedApplicationsHealthEvaluation = reinterpret_cast<FABRIC_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);
    
    auto hr = StringUtility::LpcwstrToWstring(publicUDDeployedApplicationsHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicUDDeployedApplicationsHealthEvaluation->UpgradeDomainName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, upgradeDomainName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr); 
        Trace.WriteInfo(TraceSource, "Error parsing upgrade domain name in FromPublicAPI: {0}", error);
        return error;
    }

    maxPercentUnhealthyDeployedApplications_ = publicUDDeployedApplicationsHealthEvaluation->MaxPercentUnhealthyDeployedApplications;

    aggregatedHealthState_ = publicUDDeployedApplicationsHealthEvaluation->AggregatedHealthState;

    totalCount_ = publicUDDeployedApplicationsHealthEvaluation->TotalCount;

    // DeployedApplicationsPerUD
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        publicUDDeployedApplicationsHealthEvaluation->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void UDDeployedApplicationsHealthEvaluation::SetDescription()
{
    size_t unhealthyCount = unhealthyEvaluations_.size();
    description_ = wformatString(
        HMResource::GetResources().HealthEvaluationUnhealthyDeployedApplicationsPerUDPolicy,
        GetUnhealthyPercent(unhealthyCount, totalCount_),
        unhealthyCount,
        totalCount_,
        upgradeDomainName_,
        maxPercentUnhealthyDeployedApplications_);
}
