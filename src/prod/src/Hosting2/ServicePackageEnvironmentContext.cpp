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
#if defined(PLATFORM_UNIX)
    containerGroupIsolated_(false),
#endif
    setupContainerGroup_(false),
    firewallPorts_()
{
}

ServicePackageInstanceEnvironmentContext::~ServicePackageInstanceEnvironmentContext()
{
}

void ServicePackageInstanceEnvironmentContext::AddNetworks(NetworkType::Enum networkType, std::vector<std::wstring> networkNames)
{
    auto iter = associatedNetworks_.find(networkType);
    if (iter == associatedNetworks_.end())
    {
        associatedNetworks_.insert(make_pair(networkType, std::vector<wstring>()));
        iter = associatedNetworks_.find(networkType);
    }
    iter->second = networkNames;
}

void ServicePackageInstanceEnvironmentContext::GetNetworks(NetworkType::Enum networkType, std::vector<std::wstring> & networkNames)
{
    auto iter = associatedNetworks_.find(networkType);
    if (iter != associatedNetworks_.end())
    {
        networkNames = iter->second;
    }
}

bool ServicePackageInstanceEnvironmentContext::NetworkExists(NetworkType::Enum networkType)
{
    auto iter = associatedNetworks_.find(networkType);
    return (iter != associatedNetworks_.end() && !iter->second.empty());
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
    firewallPorts_.clear();
}

void ServicePackageInstanceEnvironmentContext::AddAssignedIpAddresses(wstring const & codePackageName, wstring const & ipAddress)
{
    ipAddresses_[codePackageName] = ipAddress;
}

void ServicePackageInstanceEnvironmentContext::AddAssignedOverlayNetworkResources(wstring const & codePackageName, std::map<std::wstring, std::wstring> & overlayNetworkResources)
{
    overlayNetworkResources_.insert(make_pair(codePackageName, std::map<wstring, wstring>()));
    auto codePkgNameIter = overlayNetworkResources_.find(codePackageName);
    if (codePkgNameIter != overlayNetworkResources_.end())
    {
        for (auto const & onr : overlayNetworkResources)
        {
            codePkgNameIter->second.insert(make_pair(onr.first, onr.second));
        }
    }
}

ErrorCode ServicePackageInstanceEnvironmentContext::GetNetworkResourceAssignedToCodePackage(
    std::wstring const & codePackageName,
    __out std::wstring & openNetworkIpAddress,
    __out std::map<std::wstring, std::wstring> & overlayNetworkResources)
{
    if (setupContainerGroup_)
    {
        ServiceModel::NetworkType::Enum networkType;
        std::wstring networkResourceList;
        GetGroupAssignedNetworkResource(
            openNetworkIpAddress,
            overlayNetworkResources,
            networkType,
            networkResourceList);
    }
    else
    {
        auto it = ipAddresses_.find(codePackageName);
        if (it != ipAddresses_.end())
        {
            openNetworkIpAddress = it->second;
        }

        auto nr = overlayNetworkResources_.find(codePackageName);
        if (nr != overlayNetworkResources_.end())
        {
            for (auto const & onr : nr->second)
            {
                overlayNetworkResources.insert(make_pair(onr.first, onr.second));
            }
        }
    }

    if (!openNetworkIpAddress.empty() || (!overlayNetworkResources_.empty()))
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::NotFound;
}

void ServicePackageInstanceEnvironmentContext::GetCodePackageOverlayNetworkNames(__out std::map<wstring, vector<wstring>> & codePackageNetworkNames)
{
    if (!overlayNetworkResources_.empty())
    {
        for (auto const & overlayNetworkResource : overlayNetworkResources_)
        {
            codePackageNetworkNames.insert(make_pair(overlayNetworkResource.first, vector<wstring>()));
            for (auto const & onr : overlayNetworkResource.second)
            {
                codePackageNetworkNames[overlayNetworkResource.first].push_back(onr.first);
            }
        }
    }
}

void ServicePackageInstanceEnvironmentContext::AddGroupContainerName(wstring const & containerName)
{
    groupContainerName_ = containerName;
}

#if defined(PLATFORM_UNIX)
void ServicePackageInstanceEnvironmentContext::SetContainerGroupIsolatedFlag(bool isIsolated)
{
    containerGroupIsolated_ = isIsolated;
}
#endif

void ServicePackageInstanceEnvironmentContext::SetContainerGroupSetupFlag(bool isEnabled)
{
    setupContainerGroup_ = isEnabled;
}

void ServicePackageInstanceEnvironmentContext::GetGroupAssignedNetworkResource(
    std::wstring & openNetworkIpAddress, 
    std::map<std::wstring, std::wstring> & overlayNetworkResources,
    ServiceModel::NetworkType::Enum & networkType,
    std::wstring & networkResourceList)
{
    if (!ipAddresses_.empty())
    {
        openNetworkIpAddress = ipAddresses_.begin()->second;
        if (!openNetworkIpAddress.empty())
        {
            networkType = networkType | ServiceModel::NetworkType::Enum::Open;

            networkResourceList = (networkResourceList.empty()) 
                ? (wformatString("Open Network Ip={0}", openNetworkIpAddress)) 
                : (wformatString("{0}, Open Network Ip={1}", 
                    networkResourceList, 
                    openNetworkIpAddress));
        }
    }
   
    if (!overlayNetworkResources_.empty())
    {
        auto networkResourceMap = overlayNetworkResources_.begin()->second;
        if (!networkResourceMap.empty())
        {
            for (auto const & onr : networkResourceMap)
            {
                overlayNetworkResources.insert(make_pair(onr.first, onr.second));

                vector<wstring> assignedOverlayNetworkResourceParts;
                StringUtility::Split<wstring>(onr.second, assignedOverlayNetworkResourceParts, L",");

                networkResourceList = (networkResourceList.empty())
                    ? (wformatString("Overlay Network Ip={0}, Overlay Network Mac={1}", 
                        assignedOverlayNetworkResourceParts[0], 
                        assignedOverlayNetworkResourceParts[1]))
                    : (wformatString("{0}, Overlay Network Ip={1}, Overlay Network Mac={2}", 
                        networkResourceList, 
                        assignedOverlayNetworkResourceParts[0],
                        assignedOverlayNetworkResourceParts[1]));
            }

            if (overlayNetworkResources.size() > 0)
            {
                networkType = networkType | ServiceModel::NetworkType::Enum::Isolated;
            }
        }
    }
}