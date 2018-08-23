// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationEntity");

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationEntityHealthInformation)

DeployedApplicationEntityHealthInformation::DeployedApplicationEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION)
    , applicationName_()
    , nodeId_()
    , nodeName_()
    , applicationInstanceId_(FABRIC_INVALID_INSTANCE_ID)
{
}

DeployedApplicationEntityHealthInformation::DeployedApplicationEntityHealthInformation(
    std::wstring const & applicationName,
    Common::LargeInteger const & nodeId,
    std::wstring const & nodeName,
    FABRIC_INSTANCE_ID applicationInstanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION)
    , applicationName_(applicationName)
    , nodeId_(nodeId)
    , nodeName_(nodeName)
    , applicationInstanceId_(applicationInstanceId)
{
}

std::wstring const & DeployedApplicationEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
        entityId_ = wformatString(
            "{0}{1}{2}", 
            applicationName_, 
            EntityHealthInformation::Delimiter, 
            nodeId_);
    }

    return entityId_;
}

ErrorCode DeployedApplicationEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto deployedApplicationHealthReport = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_REPORT>();

    deployedApplicationHealthReport->ApplicationName = heap.AddString(applicationName_);
    deployedApplicationHealthReport->NodeName = heap.AddString(nodeName_);
    deployedApplicationHealthReport->HealthInformation = commonHealthInformation;

    healthReport.Kind = kind_;
    healthReport.Value = deployedApplicationHealthReport.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode DeployedApplicationEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    kind_ = healthReport.Kind;

    auto deployedApplicationHealthReport = reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_HEALTH_REPORT *>(healthReport.Value);

    auto hr = StringUtility::LpcwstrToWstring(deployedApplicationHealthReport->ApplicationName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(deployedApplicationHealthReport->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    auto error = GenerateNodeId();
    if (!error.IsSuccess())
    {
        return error;
    }
    
    attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeName_);

    return commonHealthInformation.FromCommonPublicApi(*deployedApplicationHealthReport->HealthInformation);
}

Common::ErrorCode DeployedApplicationEntityHealthInformation::GenerateNodeId()
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
