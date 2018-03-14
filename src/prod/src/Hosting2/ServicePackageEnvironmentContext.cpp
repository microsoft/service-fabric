// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackageInstanceEnvironmentContext::ServicePackageInstanceEnvironmentContext(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
    : servicePackageInstanceId_(servicePackageInstanceId),
    groupContainerName_(),
    setupContainerGroup_(false)
{
}

ServicePackageInstanceEnvironmentContext::~ServicePackageInstanceEnvironmentContext()
{
}

void ServicePackageInstanceEnvironmentContext::AddEndpoint(EndpointResourceSPtr && endpointResource)
{            
    endpointResources_.push_back(move(endpointResource));
}

void ServicePackageInstanceEnvironmentContext::AddEtwProviderGuid(wstring const & etwProviderGuid)
{            
    etwProviderGuids_.push_back(etwProviderGuid);
}

void ServicePackageInstanceEnvironmentContext::AddCertificatePaths(std::map<std::wstring, std::wstring> const & certificatePaths, std::map<std::wstring, std::wstring> const & certificatePasswordPaths)
{
    certificatePaths_ = certificatePaths;
    certificatePasswordPaths_= certificatePasswordPaths;
}

void ServicePackageInstanceEnvironmentContext::Reset()
{    
    endpointResources_.clear();    
    etwProviderGuids_.clear();
}

void ServicePackageInstanceEnvironmentContext::AddAssignedIpAddresses(wstring const & codePackageName, wstring const & ipAddress)
{
    ipAddresses_[codePackageName] = ipAddress;
}

ErrorCode ServicePackageInstanceEnvironmentContext::GetIpAddressAssignedToCodePackage(
    wstring const & codePackageName,
    __out wstring & ipAddress)
{
    if (setupContainerGroup_)
    {
        ipAddress = this->GetGroupAssignedIp();
        if (!ipAddress.empty())
        {
            return ErrorCodeValue::Success;
        }
    }
    else
    {
        auto it = ipAddresses_.find(codePackageName);
        if (it != ipAddresses_.end())
        {
            ipAddress = it->second;
            return ErrorCodeValue::Success;
        }
    }
    return ErrorCodeValue::NotFound;
}

void ServicePackageInstanceEnvironmentContext::AddGroupContainerName(wstring const & containerName)
{
    groupContainerName_ = containerName;
}

void ServicePackageInstanceEnvironmentContext::SetContainerGroupSetupFlag(bool isEnabled)
{
    setupContainerGroup_ = isEnabled;
}

wstring ServicePackageInstanceEnvironmentContext::GetGroupAssignedIp()
{
    wstring ipAddress;
    if (!ipAddresses_.empty())
    {
        ipAddress = ipAddresses_.begin()->second;
    }
    return ipAddress;
}
