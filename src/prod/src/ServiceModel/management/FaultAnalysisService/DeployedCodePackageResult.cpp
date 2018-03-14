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

StringLiteral const TraceComponent("DeployedCodePackageResult");

DeployedCodePackageResult::DeployedCodePackageResult()
: nodeName_(L"")
, applicationName_()
, serviceManifestName_(L"")
, servicePackageActivationId_(L"")
, codePackageName_(L"")
, codePackageInstanceId_()
{
}

DeployedCodePackageResult::DeployedCodePackageResult(DeployedCodePackageResult && other)
: nodeName_(move(other.nodeName_))
, applicationName_(move(other.applicationName_))
, serviceManifestName_(move(other.serviceManifestName_))
, servicePackageActivationId_(move(other.servicePackageActivationId_))
, codePackageName_(move(other.codePackageName_))
, codePackageInstanceId_(move(other.codePackageInstanceId_))
{
}

ErrorCode DeployedCodePackageResult::FromPublicApi(FABRIC_DEPLOYED_CODE_PACKAGE_RESULT const & publicResult)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicResult.NodeName, true, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "DeployedCodePackageResult::FromPublicApi/Failed to parse NodeName");
        return ErrorCode::FromHResult(hr);
    }

    hr = NamingUri::TryParse(publicResult.ApplicationName, L"TraceId", applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "DeployedCodePackageResult::FromPublicApi/Failed to parse ApplicationName");
        return ErrorCodeValue::InvalidNameUri;
    }

    hr = StringUtility::LpcwstrToWstring(publicResult.ServiceManifestName, true, serviceManifestName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "DeployedCodePackageResult::FromPublicApi/Failed to parse ServiceManifestName");
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(publicResult.CodePackageName, true, codePackageName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "DeployedCodePackageResult::FromPublicApi/Failed to parse CodePackageName");
        return ErrorCode::FromHResult(hr);
    }

    if (publicResult.Reserved != nullptr)
    {
        auto ex1 = (FABRIC_DEPLOYED_CODE_PACKAGE_RESULT_EX1*)publicResult.Reserved;

        hr = StringUtility::LpcwstrToWstring(ex1->ServicePackageActivationId, true, servicePackageActivationId_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "DeployedCodePackageResult::FromPublicApi/Failed to parse ServicePackageActivationId.");
            return ErrorCode::FromHResult(hr);
        }
    }

    codePackageInstanceId_ = publicResult.CodePackageInstanceId;

    return ErrorCodeValue::Success;
}

void DeployedCodePackageResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_DEPLOYED_CODE_PACKAGE_RESULT & publicResult) const
{
    publicResult.NodeName = heap.AddString(nodeName_);

    publicResult.ApplicationName = heap.AddString(applicationName_.Name);

    publicResult.ServiceManifestName = heap.AddString(serviceManifestName_);

    publicResult.CodePackageName = heap.AddString(codePackageName_);

    publicResult.CodePackageInstanceId = codePackageInstanceId_;

    auto ex1 = heap.AddItem<FABRIC_DEPLOYED_CODE_PACKAGE_RESULT_EX1>();
    ex1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    ex1->Reserved = nullptr;

    publicResult.Reserved = ex1.GetRawPointer();
}

