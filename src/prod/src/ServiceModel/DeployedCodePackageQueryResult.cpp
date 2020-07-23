// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeployedCodePackageQueryResult::DeployedCodePackageQueryResult()
    : codePackageName_()    
    , codePackageVersion_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , hostType_(HostType::Invalid)
    , hostIsolationMode_(HostIsolationMode::None)
    , deployedCodePackageStatus_(DeploymentStatus::Invalid)
    , runFrequencyInterval_(0)
    , hasSetupEntryPoint_(false)
    , setupEntryPoint_()
    , mainEntryPoint_()
    , nodeName_()
    , serviceNameInternalUseOnly_()
{
}

DeployedCodePackageQueryResult::DeployedCodePackageQueryResult(
    std::wstring const & codePackageName,
    std::wstring const & codePackageVersion,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    HostType::Enum hostType,
    HostIsolationMode::Enum hostIsolationMode,
    DeploymentStatus::Enum deployedCodePackageStatus,
    ULONG runFrequencyInterval,
    bool hasSetupEntryPoint,
    CodePackageEntryPoint && mainEntryPoint,
    CodePackageEntryPoint && setupEntryPoint,
    std::wstring const & serviceNameInternalUseOnly)
    : codePackageName_(codePackageName)
    , codePackageVersion_(codePackageVersion)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , hostType_(hostType)
    , hostIsolationMode_(hostIsolationMode)
    , deployedCodePackageStatus_(deployedCodePackageStatus)
    , runFrequencyInterval_(runFrequencyInterval)
    , hasSetupEntryPoint_(hasSetupEntryPoint)
    , setupEntryPoint_(move(setupEntryPoint))
    , mainEntryPoint_(move(mainEntryPoint))
    , nodeName_()
    , serviceNameInternalUseOnly_(serviceNameInternalUseOnly)
{
}

void DeployedCodePackageQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedCodePackageQueryResult) const 
{
    publicDeployedCodePackageQueryResult.CodePackageName = heap.AddString(codePackageName_);
    publicDeployedCodePackageQueryResult.CodePackageVersion = heap.AddString(codePackageVersion_);
    publicDeployedCodePackageQueryResult.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedCodePackageQueryResult.DeployedCodePackageStatus = DeploymentStatus::ToPublicApi(deployedCodePackageStatus_);
    publicDeployedCodePackageQueryResult.RunFrequencyInterval = runFrequencyInterval_;

    auto mainEntryPoint = heap.AddItem<FABRIC_CODE_PACKAGE_ENTRY_POINT>();
    mainEntryPoint_.ToPublicApi(heap, *mainEntryPoint);
    publicDeployedCodePackageQueryResult.EntryPoint = mainEntryPoint.GetRawPointer();

    if (!hasSetupEntryPoint_)
    {
        publicDeployedCodePackageQueryResult.SetupEntryPoint = nullptr;
    }
    else
    {
        auto setupEntryPoint = heap.AddItem<FABRIC_CODE_PACKAGE_ENTRY_POINT>();
        setupEntryPoint_.ToPublicApi(heap, *setupEntryPoint);
        publicDeployedCodePackageQueryResult.SetupEntryPoint = setupEntryPoint.GetRawPointer();
    }
    
    auto queryResultEx1 = heap.AddItem<FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_EX1>();
    
    queryResultEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    queryResultEx1->HostType = HostType::ToPublicApi(hostType_);
    queryResultEx1->HostIsolationMode = HostIsolationMode::ToPublicApi(hostIsolationMode_);
    queryResultEx1->Reserved = nullptr;
    publicDeployedCodePackageQueryResult.Reserved = queryResultEx1.GetRawPointer();
}

void DeployedCodePackageQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring DeployedCodePackageQueryResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedCodePackageQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

Common::ErrorCode DeployedCodePackageQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM const & publicDeployedCodePackageQueryResult)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedCodePackageQueryResult.CodePackageName, true, codePackageName_);
    if (!error.IsSuccess())
    {
        return error;
    }
    
    error = StringUtility::LpcwstrToWstring2(publicDeployedCodePackageQueryResult.CodePackageVersion, true, codePackageVersion_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(publicDeployedCodePackageQueryResult.ServiceManifestName, true, serviceManifestName_);
    if (!error.IsSuccess())
    {
        return error;
    }

    runFrequencyInterval_ = publicDeployedCodePackageQueryResult.RunFrequencyInterval;

    error = DeploymentStatus::FromPublicApi(publicDeployedCodePackageQueryResult.DeployedCodePackageStatus, deployedCodePackageStatus_);
    if(!error.IsSuccess()) { return error; }

    hasSetupEntryPoint_ = false;
    if (publicDeployedCodePackageQueryResult.SetupEntryPoint != NULL)
    {
        hasSetupEntryPoint_ = true;
        error = setupEntryPoint_.FromPublicApi(*publicDeployedCodePackageQueryResult.SetupEntryPoint);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    error = mainEntryPoint_.FromPublicApi(*publicDeployedCodePackageQueryResult.EntryPoint);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (publicDeployedCodePackageQueryResult.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto queryResultEx1 = (FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_EX1*)publicDeployedCodePackageQueryResult.Reserved;

    error = StringUtility::LpcwstrToWstring2(queryResultEx1->ServicePackageActivationId, true, servicePackageActivationId_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = HostType::FromPublicApi(queryResultEx1->HostType, hostType_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = HostIsolationMode::FromPublicApi(queryResultEx1->HostIsolationMode, hostIsolationMode_);
    if (!error.IsSuccess())
    {
        return error;
    }

    return error;
}
