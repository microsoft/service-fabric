// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("RestartDeployedCodePackageDescriptionUsingNodeName");

RestartDeployedCodePackageDescriptionUsingNodeName::RestartDeployedCodePackageDescriptionUsingNodeName()
	: nodeName_(L"")
	, applicationName_()
	, serviceManifestName_(L"")
	, servicePackageActivationId_(L"")
	, codePackageName_(L"")
	, codePackageInstanceId_()
{
}

RestartDeployedCodePackageDescriptionUsingNodeName::RestartDeployedCodePackageDescriptionUsingNodeName(
	wstring const & nodeName,
	NamingUri const & applicationName,
	wstring const & serviceManifestName,
	wstring const & servicePackageActivationId,
	wstring const & codePackageName,
	FABRIC_INSTANCE_ID codePackageInstanceId)
	: nodeName_(nodeName)
	, applicationName_(applicationName)
	, serviceManifestName_(serviceManifestName)
	, servicePackageActivationId_(servicePackageActivationId)
	, codePackageName_(codePackageName)
	, codePackageInstanceId_(codePackageInstanceId)
{
}

RestartDeployedCodePackageDescriptionUsingNodeName::RestartDeployedCodePackageDescriptionUsingNodeName(RestartDeployedCodePackageDescriptionUsingNodeName const & other)
: nodeName_(other.nodeName_)
, applicationName_(other.applicationName_)
, serviceManifestName_(other.serviceManifestName_)
, servicePackageActivationId_(other.servicePackageActivationId_)
, codePackageName_(other.codePackageName_)
, codePackageInstanceId_(other.codePackageInstanceId_)
{
}

RestartDeployedCodePackageDescriptionUsingNodeName::RestartDeployedCodePackageDescriptionUsingNodeName(RestartDeployedCodePackageDescriptionUsingNodeName && other)
: nodeName_(move(other.nodeName_))
, applicationName_(move(other.applicationName_))
, serviceManifestName_(move(other.serviceManifestName_))
, servicePackageActivationId_(move(other.servicePackageActivationId_))
, codePackageName_(move(other.codePackageName_))
, codePackageInstanceId_(move(other.codePackageInstanceId_))
{
}

Common::ErrorCode RestartDeployedCodePackageDescriptionUsingNodeName::FromPublicApi(
    FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    hr = NamingUri::TryParse(publicDescription.ApplicationName, L"TraceId", applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "RestartDeployedCodePackageDescriptionUsingNodeName::FromPublicApi/Failed to parse ApplicationName");
        return ErrorCodeValue::InvalidNameUri;
    }

    hr = StringUtility::LpcwstrToWstring(publicDescription.ServiceManifestName, true, serviceManifestName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "RestartDeployedCodePackageDescriptionUsingNodeName::FromPublicApi/Failed to parse ServiceManifestName");
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(publicDescription.CodePackageName, true, codePackageName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "RestartDeployedCodePackageDescriptionUsingNodeName::FromPublicApi/Failed to parse CodePackageName");
        return ErrorCode::FromHResult(hr);
    }

    codePackageInstanceId_ = publicDescription.CodePackageInstanceId;

    if (publicDescription.Reserved != nullptr)
    {
        auto ex1 = (FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME_EX1*)publicDescription.Reserved;

        hr = StringUtility::LpcwstrToWstring(ex1->ServicePackageActivationId, true, servicePackageActivationId_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "RestartDeployedCodePackageDescriptionUsingNodeName::FromPublicApi/Failed to parse ServicePackageActivationId.");
            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCodeValue::Success;
}

void RestartDeployedCodePackageDescriptionUsingNodeName::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME & result) const
{
    result.NodeName = heap.AddString(nodeName_);

    result.ApplicationName = heap.AddString(applicationName_.Name);

    result.ServiceManifestName = heap.AddString(serviceManifestName_);

    result.CodePackageName = heap.AddString(codePackageName_);

    result.CodePackageInstanceId = codePackageInstanceId_;

    auto ex1 = heap.AddItem<FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME_EX1>();
    ex1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    ex1->Reserved = nullptr;

    result.Reserved = ex1.GetRawPointer();
}
