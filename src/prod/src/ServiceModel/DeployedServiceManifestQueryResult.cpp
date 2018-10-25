// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeployedServiceManifestQueryResult::DeployedServiceManifestQueryResult()
    : serviceManifestName_()
    , servicePackageActivationId_()
    , serviceManifestVersion_()
    , deployedServiceManifestStatus_(DeploymentStatus::Invalid)
{
}

DeployedServiceManifestQueryResult::DeployedServiceManifestQueryResult(
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & serviceManifestVersion,
    DeploymentStatus::Enum deployedServiceManifestStatus)
    : serviceManifestName_(serviceManifestName)   
    , servicePackageActivationId_(servicePackageActivationId)
    , serviceManifestVersion_(serviceManifestVersion) 
    , deployedServiceManifestStatus_(deployedServiceManifestStatus)
{
}

void DeployedServiceManifestQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedServiceManifestQueryResult) const 
{
    publicDeployedServiceManifestQueryResult.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedServiceManifestQueryResult.ServiceManifestVersion = heap.AddString(serviceManifestVersion_);
    publicDeployedServiceManifestQueryResult.DeployedServicePackageStatus = DeploymentStatus::ToPublicApi(deployedServiceManifestStatus_);

    auto queryResultEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM_EX1>();

    queryResultEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    queryResultEx1->Reserved = nullptr;
    publicDeployedServiceManifestQueryResult.Reserved = queryResultEx1.GetRawPointer(); 
}

void DeployedServiceManifestQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring DeployedServiceManifestQueryResult::ToString() const
{
    return wformatString(
        "ServiceManifestName = {0}, ServicePackageActivationId = {1}, ServiceManifestVersion = {2}, DeployedServicePackageStatus = {3}\n", 
        serviceManifestName_,
        servicePackageActivationId_,
        serviceManifestVersion_,
        deployedServiceManifestStatus_);
}

Common::ErrorCode DeployedServiceManifestQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM const & publicDeployedServiceManifestQueryResult)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedServiceManifestQueryResult.ServiceManifestName, true, serviceManifestName_);
    if (!error.IsSuccess())
    {
        return error;
    }
    
    error = StringUtility::LpcwstrToWstring2(publicDeployedServiceManifestQueryResult.ServiceManifestVersion, true, serviceManifestVersion_);
    if (!error.IsSuccess())
    {
        return error;
    }
    
    error = DeploymentStatus::FromPublicApi(publicDeployedServiceManifestQueryResult.DeployedServicePackageStatus, deployedServiceManifestStatus_);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (publicDeployedServiceManifestQueryResult.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto queryResultEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM_EX1*)publicDeployedServiceManifestQueryResult.Reserved;

    error = StringUtility::LpcwstrToWstring2(queryResultEx1->ServicePackageActivationId, true, servicePackageActivationId_);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}
