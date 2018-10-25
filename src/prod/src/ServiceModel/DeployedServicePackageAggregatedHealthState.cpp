// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageAggregatedHealthState)

StringLiteral const TraceSource("DeployedServicePackageAggregatedHealthState");

DeployedServicePackageAggregatedHealthState::DeployedServicePackageAggregatedHealthState()
    : applicationName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , nodeName_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

DeployedServicePackageAggregatedHealthState::DeployedServicePackageAggregatedHealthState(
    std::wstring const & applicationName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : applicationName_(applicationName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , nodeName_(nodeName)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

DeployedServicePackageAggregatedHealthState::~DeployedServicePackageAggregatedHealthState()
{
}

Common::ErrorCode DeployedServicePackageAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE & publicDeployedServicePackageAggregatedHealthState) const 
{
    // Entity information
    publicDeployedServicePackageAggregatedHealthState.ApplicationName = heap.AddString(applicationName_);
    publicDeployedServicePackageAggregatedHealthState.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedServicePackageAggregatedHealthState.NodeName = heap.AddString(nodeName_);

    // Health state
    publicDeployedServicePackageAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;
    
    auto queryResultEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_EX1>();

    queryResultEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    queryResultEx1->Reserved = nullptr;
    publicDeployedServicePackageAggregatedHealthState.Reserved = queryResultEx1.GetRawPointer();

    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackageAggregatedHealthState::FromPublicApi(
    FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE const & publicDeployedServicePackageAggregatedHealthState)
{
    // Entity information
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageAggregatedHealthState.ApplicationName, false, applicationName_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }

	error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageAggregatedHealthState.ServiceManifestName, false, serviceManifestName_);
	if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing service manifest name in FromPublicAPI: {0}", error);
        return error;
    }

	error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageAggregatedHealthState.NodeName, false, nodeName_);
	if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

    // Health state
    aggregatedHealthState_ = publicDeployedServicePackageAggregatedHealthState.AggregatedHealthState;

    if (publicDeployedServicePackageAggregatedHealthState.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto queryResultEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_EX1*)publicDeployedServicePackageAggregatedHealthState.Reserved;

	error = StringUtility::LpcwstrToWstring2(queryResultEx1->ServicePackageActivationId, true, servicePackageActivationId_);
	if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing servicePackageActivationId in FromPublicAPI: {0}", error);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void DeployedServicePackageAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write(
        "DeployedServicePackageAggregatedHealthState({0}+{1}+{2}+{3}: {4})",
        applicationName_, 
        serviceManifestName_,
        servicePackageActivationId_,
        nodeName_, 
        aggregatedHealthState_);
}

std::wstring DeployedServicePackageAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring DeployedServicePackageAggregatedHealthState::CreateContinuationToken() const
{
	return CreateContinuationString(serviceManifestName_, servicePackageActivationId_);
}

wstring DeployedServicePackageAggregatedHealthState::CreateContinuationString(
	wstring const & serviceManifestName,
	wstring const & servicePackageActivationId)
{
	if (!servicePackageActivationId.empty())
	{
		return wformatString("{0}_{1}", serviceManifestName, servicePackageActivationId);
	}

	return serviceManifestName;
}
