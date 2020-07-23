// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("DeployedNetworkCodePackageQueryResult");

DeployedNetworkCodePackageQueryResult::DeployedNetworkCodePackageQueryResult()
    : applicationName_()
    , networkName_()
    , codePackageName_()
    , codePackageVersion_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , containerAddress_()
    , containerId_()
{
}

DeployedNetworkCodePackageQueryResult::DeployedNetworkCodePackageQueryResult(
    std::wstring const & applicationName,
    std::wstring const & networkName,
    std::wstring const & codePackageName,
    std::wstring const & codePackageVersion,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    std::wstring const & containerId,
    std::wstring const & containerAddress)
    : applicationName_(applicationName)
    , networkName_(networkName)
    , codePackageName_(codePackageName)
    , codePackageVersion_(codePackageVersion)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , containerId_(containerId)
    , containerAddress_(containerAddress)
{
}

DeployedNetworkCodePackageQueryResult::DeployedNetworkCodePackageQueryResult(DeployedNetworkCodePackageQueryResult && other)
    : applicationName_(move(other.applicationName_))
    , networkName_(move(other.networkName_))
    , codePackageName_(move(other.codePackageName_))
    , codePackageVersion_(move(other.codePackageVersion_))
    , serviceManifestName_(move(other.serviceManifestName_))
    , servicePackageActivationId_(move(other.servicePackageActivationId_))
    , containerAddress_(move(other.containerAddress_))
    , containerId_(move(other.containerId_))
{
}

DeployedNetworkCodePackageQueryResult const & DeployedNetworkCodePackageQueryResult::operator = (DeployedNetworkCodePackageQueryResult && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
        networkName_ = move(other.networkName_);
        codePackageName_ = move(other.codePackageName_);
        codePackageVersion_ = move(other.codePackageVersion_);
        serviceManifestName_ = move(other.serviceManifestName_);
        servicePackageActivationId_ = move(other.servicePackageActivationId_);
        containerAddress_ = move(other.containerAddress_);
        containerId_ = move(other.containerId_);
    }

    return *this;
}

std::wstring DeployedNetworkCodePackageQueryResult::ToString() const
{
    return wformatString(
        "ApplicationName=[{0}], NetworkName=[{1}], CodePackageName=[{2}], CodePackageVersion=[{3}], ServiceManifestName=[{4}], ServicePackageActivationId=[{5}], ContainerId=[{6}], ContainerAddress=[{7}]\n",
        applicationName_,
        networkName_,
        codePackageName_,
        codePackageVersion_,
        serviceManifestName_,
        servicePackageActivationId_,
        containerId_,
        containerAddress_);
}

Common::ErrorCode DeployedNetworkCodePackageQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM const &deployedNetworkCodePackageQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.ApplicationName, false, applicationName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.NetworkName, false, networkName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.CodePackageName, false, codePackageName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.CodePackageVersion, false, codePackageVersion_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.ServiceManifestName, false, serviceManifestName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.ServicePackageActivationId, false, servicePackageActivationId_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.ContainerAddress, false, containerAddress_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(deployedNetworkCodePackageQueryResult.ContainerId, false, containerId_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void DeployedNetworkCodePackageQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedNetworkCodePackageQueryResult) const
{
    publicDeployedNetworkCodePackageQueryResult.ApplicationName = heap.AddString(applicationName_);
    publicDeployedNetworkCodePackageQueryResult.NetworkName = heap.AddString(networkName_);
    publicDeployedNetworkCodePackageQueryResult.CodePackageName = heap.AddString(codePackageName_);
    publicDeployedNetworkCodePackageQueryResult.CodePackageVersion = heap.AddString(codePackageVersion_);
    publicDeployedNetworkCodePackageQueryResult.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedNetworkCodePackageQueryResult.ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    publicDeployedNetworkCodePackageQueryResult.ContainerAddress = heap.AddString(containerAddress_);
    publicDeployedNetworkCodePackageQueryResult.ContainerId = heap.AddString(containerId_);
    publicDeployedNetworkCodePackageQueryResult.Reserved = nullptr;
}
