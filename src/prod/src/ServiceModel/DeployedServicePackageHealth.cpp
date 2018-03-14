// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageHealth)

StringLiteral const TraceSource("DeployedServicePackageHealth");

DeployedServicePackageHealth::DeployedServicePackageHealth()
    : EntityHealthBase()
    , applicationName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , nodeName_()
{
}

DeployedServicePackageHealth::DeployedServicePackageHealth(
    std::wstring const & applicationName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & nodeName,
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations)
    : EntityHealthBase(move(events), aggregatedHealthState, move(unhealthyEvaluations))
    , applicationName_(applicationName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , nodeName_(nodeName)
{
}

DeployedServicePackageHealth::~DeployedServicePackageHealth()
{
}

Common::ErrorCode DeployedServicePackageHealth::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH & publicDeployedServicePackageHealth) const 
{
    ErrorCode error(ErrorCodeValue::Success);

    // Entity information
    publicDeployedServicePackageHealth.ApplicationName = heap.AddString(applicationName_);
    publicDeployedServicePackageHealth.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedServicePackageHealth.NodeName = heap.AddString(nodeName_);

    // Health state
    publicDeployedServicePackageHealth.AggregatedHealthState = aggregatedHealthState_;
    
    // Events
    auto publicHealthEventList = heap.AddItem<FABRIC_HEALTH_EVENT_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT, FABRIC_HEALTH_EVENT_LIST>(
        heap,
        events_, 
        *publicHealthEventList);
    if (!error.IsSuccess()) { return error; }
    publicDeployedServicePackageHealth.HealthEvents = publicHealthEventList.GetRawPointer();
    
    // Evaluation reasons
    auto publicHealthEvaluationList = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationList);
    if (!error.IsSuccess()) { return error; }

    auto publicDeployedServicePackageHealthEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX1>();
    publicDeployedServicePackageHealthEx1->UnhealthyEvaluations = publicHealthEvaluationList.GetRawPointer();
    publicDeployedServicePackageHealth.Reserved = publicDeployedServicePackageHealthEx1.GetRawPointer();

    auto publicDeployedServicePackageHealthEx2 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX2>();

    publicDeployedServicePackageHealthEx2->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    publicDeployedServicePackageHealthEx2->Reserved = nullptr;
    publicDeployedServicePackageHealthEx1->Reserved = publicDeployedServicePackageHealthEx2.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackageHealth::FromPublicApi(
    FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH const & publicDeployedServicePackageHealth)
{
    // Entity information
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealth.ApplicationName, false, applicationName_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing deployedServicePackage name in FromPublicAPI: {0}", error);
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealth.ServiceManifestName, false, serviceManifestName_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing service manifest name in FromPublicAPI: {0}", error);
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealth.NodeName, false, nodeName_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

    // Health state
    aggregatedHealthState_ = publicDeployedServicePackageHealth.AggregatedHealthState;
    
    // Events
    error = PublicApiHelper::FromPublicApiList<HealthEvent, FABRIC_HEALTH_EVENT_LIST>(publicDeployedServicePackageHealth.HealthEvents, events_);
    if (!error.IsSuccess()) { return error; }

    if (publicDeployedServicePackageHealth.Reserved == NULL)
    {
        return ErrorCode::Success();
    }

    auto deployedServicePackageHealthEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX1*)publicDeployedServicePackageHealth.Reserved;

    // Evaluation reasons
    error = PublicApiHelper::FromPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION_LIST>(
        deployedServicePackageHealthEx1->UnhealthyEvaluations,
        unhealthyEvaluations_);
    if (!error.IsSuccess()) { return error; }
    
    if (deployedServicePackageHealthEx1->Reserved == nullptr)
    {
        return ErrorCode::Success();
    }

    auto deployedServicePackageHealthEx2 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX2*)deployedServicePackageHealthEx1->Reserved;

    error = StringUtility::LpcwstrToWstring2(deployedServicePackageHealthEx2->ServicePackageActivationId, true, servicePackageActivationId_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing ServicePackageActivationId in FromPublicAPI: {0}", error);
        return error;
    }

    return error;
}
