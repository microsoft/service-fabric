// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackageEntity");

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageEntityHealthInformation)

DeployedServicePackageEntityHealthInformation::DeployedServicePackageEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE)
    , applicationName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , nodeId_()
    , nodeName_()
    , servicePackageInstanceId_(FABRIC_INVALID_INSTANCE_ID)
{
}

DeployedServicePackageEntityHealthInformation::DeployedServicePackageEntityHealthInformation(
    std::wstring const & applicationName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    Common::LargeInteger const & nodeId,
    std::wstring const & nodeName,
    FABRIC_INSTANCE_ID servicePackageInstanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE)
    , applicationName_(applicationName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , nodeId_(nodeId)
    , nodeName_(nodeName)
    , servicePackageInstanceId_(servicePackageInstanceId)
{
}

std::wstring const & DeployedServicePackageEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
		if (servicePackageActivationId_.empty())
		{
			entityId_ = wformatString(
				"{0}{1}{2}{3}{4}",
				applicationName_,
				EntityHealthInformation::Delimiter,
				serviceManifestName_,
				EntityHealthInformation::Delimiter,
				nodeId_);
		}
		else
		{
			entityId_ = wformatString(
				"{0}{1}{2}{3}{4}{5}{6}",
				applicationName_,
				EntityHealthInformation::Delimiter,
				serviceManifestName_,
				EntityHealthInformation::Delimiter,
				servicePackageActivationId_,
				EntityHealthInformation::Delimiter,
				nodeId_);
		}
    }

    return entityId_;
}

ErrorCode DeployedServicePackageEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto deployedServicePackageHealthReport = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT>();
    deployedServicePackageHealthReport->ApplicationName = heap.AddString(applicationName_);
    deployedServicePackageHealthReport->ServiceManifestName = heap.AddString(serviceManifestName_);
    deployedServicePackageHealthReport->NodeName = heap.AddString(nodeName_);
    deployedServicePackageHealthReport->HealthInformation = commonHealthInformation;

    auto healthReportEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT_EX1>();

    healthReportEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    healthReportEx1->Reserved = nullptr;
    deployedServicePackageHealthReport->Reserved = healthReportEx1.GetRawPointer();

    healthReport.Kind = kind_;
    healthReport.Value = deployedServicePackageHealthReport.GetRawPointer();
    
    return ErrorCode::Success();
}

ErrorCode DeployedServicePackageEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    kind_ = healthReport.Kind;

    auto deployedServicePackageHealthReport = reinterpret_cast<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT *>(healthReport.Value);

    auto error = StringUtility::LpcwstrToWstring2(deployedServicePackageHealthReport->ApplicationName, false, applicationName_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Error parsing application name in FromPublicAPI: {0}", error);
        return error;
    }
    
	error = StringUtility::LpcwstrToWstring2(deployedServicePackageHealthReport->ServiceManifestName, false, serviceManifestName_);
	if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Error parsing service manifest name in FromPublicAPI: {0}", error);
        return error;
    }

	error = StringUtility::LpcwstrToWstring2(deployedServicePackageHealthReport->NodeName, false, nodeName_);
	if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Error parsing node name in FromPublicAPI: {0}", error);
        return error;
    }

    error = GenerateNodeId();
    if (!error.IsSuccess())
    {
        return error;
    }
    
    attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeName_);

    error = commonHealthInformation.FromCommonPublicApi(*deployedServicePackageHealthReport->HealthInformation);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (deployedServicePackageHealthReport->Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto healthReportEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT_EX1*)deployedServicePackageHealthReport->Reserved;

	error = StringUtility::LpcwstrToWstring2(healthReportEx1->ServicePackageActivationId, true, servicePackageActivationId_);
	if (!error.IsSuccess())
    {
        Trace.WriteError(TraceSource, "Error parsing ServicePackageActivationId in FromPublicAPI: {0}", error);
		return error;
    }

    return error;
}

Common::ErrorCode DeployedServicePackageEntityHealthInformation::GenerateNodeId()
{
    Federation::NodeId nodeId;
    ErrorCode error = Federation::NodeIdGenerator::GenerateFromString(nodeName_, nodeId);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error generating NodeId from NodeName {0}: {1}", nodeName_, error);
        return error;
    }

    if (nodeId != nodeId_)
    {
        Trace.WriteInfo(TraceSource, "Generate NodeId from NodeName {0}: {1} (previous {2})", nodeName_, nodeId, nodeId_);
        nodeId_ = nodeId.IdValue;
        entityId_.clear();
    }

    return ErrorCode::Success();
}
