// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerNetworkConfigDescription::ContainerNetworkConfigDescription() 
    : openNetworkAssignedIp_()
    , overlayNetworkResources_()
    , portBindings_()
    , nodeId_()
    , nodeName_()
    , nodeIpAddress_()
    , networkType_(ServiceModel::NetworkType::Enum::Other)
{
}

ContainerNetworkConfigDescription::ContainerNetworkConfigDescription(
    std::wstring const & openNetworkAssignedIp,
    std::map<std::wstring, std::wstring> const & overlayNetworkResources,
    std::map<std::wstring, std::wstring> const & portBindings,
    std::wstring const & nodeId,
    std::wstring const & nodeName,
    std::wstring const & nodeIpAddress,
    NetworkType::Enum networkType)
    : openNetworkAssignedIp_(openNetworkAssignedIp)
    , overlayNetworkResources_(overlayNetworkResources)
    , portBindings_(portBindings)
    , nodeId_(nodeId)
    , nodeName_(nodeName)
    , nodeIpAddress_(nodeIpAddress)
    , networkType_(networkType)
{
}

ErrorCode ContainerNetworkConfigDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_NETWORK_CONFIG_DESCRIPTION & fabricContainerNetworkConfig) const
{
    fabricContainerNetworkConfig.OpenNetworkAssignedIp = heap.AddString(this->OpenNetworkAssignedIp);

    auto overlayNetworkResources = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    auto error = PublicApiHelper::ToPublicApiStringPairList(heap, this->OverlayNetworkResources, *overlayNetworkResources);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerNetworkConfig.OverlayNetworkResources = overlayNetworkResources.GetRawPointer();

    auto portBindings = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    error = PublicApiHelper::ToPublicApiStringPairList(heap, this->PortBindings, *portBindings);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerNetworkConfig.PortBindings = portBindings.GetRawPointer();

    fabricContainerNetworkConfig.NetworkType = NetworkType::ToPublicApi(this->NetworkType);
    fabricContainerNetworkConfig.NodeId = heap.AddString(this->NodeId);
    fabricContainerNetworkConfig.NodeName = heap.AddString(this->NodeName);
    fabricContainerNetworkConfig.NodeIpAddress = heap.AddString(this->NodeIpAddress);

    return ErrorCode::Success();
}

void ContainerNetworkConfigDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerNetworkConfigDescription { ");
    w.Write("OpenNetworkAssignedIP = {0} ", openNetworkAssignedIp_);
    w.Write("AssignedOverlayNetworkResources { ");
    for (auto iter = overlayNetworkResources_.begin(); iter != overlayNetworkResources_.end(); ++iter)
    {
        w.Write("NetworkName = {0} ", iter->first);
        w.Write("AssignedOverlayNetworkResource = {0} ", iter->second);
    }
    w.Write("} ");
    w.Write("PortBindings { ");
    for (auto iter = portBindings_.begin(); iter != portBindings_.end(); ++iter)
    {
        w.Write("Host Port = {0} ", iter->first);
        w.Write("Image Port = {1} ", iter->second);
    }
    w.Write("} ");
    w.Write("NodeId = {0} ", NodeId);
    w.Write("NodeName = {0} ", NodeName);
    w.Write("NodeIpAddress = {0} ", NodeIpAddress);
    w.Write("NetworkType = {0} ", NetworkType);
    w.Write("}");
}

void ContainerNetworkConfigDescription::ToEnvironmentMap(Common::EnvironmentMap & envMap) const
{
    int interfaceIndex = 0;

    envMap[Common::ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable] = NetworkType::EnumToString(networkType_);

    if ((networkType_ & NetworkType::Enum::Open) == NetworkType::Enum::Open && !openNetworkAssignedIp_.empty())
    {
        envMap[wformatString(Common::ContainerEnvironment::ContainerNetworkingResourceEnvironmentVariable, interfaceIndex++, NetworkType::OpenStr)] = openNetworkAssignedIp_;
    }

    if ((networkType_ & NetworkType::Enum::Isolated) == NetworkType::Enum::Isolated && !overlayNetworkResources_.empty())
    {
        for (auto const & onr : overlayNetworkResources_)
        {
            envMap[wformatString(Common::ContainerEnvironment::ContainerNetworkingResourceEnvironmentVariable, interfaceIndex++, onr.first)] = onr.second;
        }
    }

    if (!portBindings_.empty())
    {
        envMap[wformatString(Common::ContainerEnvironment::ContainerNetworkingResourceEnvironmentVariable, interfaceIndex++, NetworkType::OtherStr)] = NetworkType::OtherStr;
    }
}

void ContainerNetworkConfigDescription::GetNetworkNames(
    __in std::vector<std::wstring> & networkTypesToFilter,
    __out std::vector<std::wstring> & networkNames,
    __out std::wstring & networkNameList) const
{
    if ((NetworkType & NetworkType::Enum::Isolated) == NetworkType::Enum::Isolated)
    {
        if ((!networkTypesToFilter.empty() &&
            std::find(networkTypesToFilter.begin(), networkTypesToFilter.end(), NetworkType::IsolatedStr) != networkTypesToFilter.end()) ||
            networkTypesToFilter.empty())
        {
            for (auto const & onr : OverlayNetworkResources)
            {
                networkNames.push_back(onr.first);
                networkNameList = (networkNameList.empty()) ? (wformatString("{0}", onr.first)) : (wformatString("{0},{1}", networkNameList, onr.first));
            }
        }
    }

    if ((NetworkType & NetworkType::Enum::Open) == NetworkType::Enum::Open)
    {
        if ((!networkTypesToFilter.empty() &&
            std::find(networkTypesToFilter.begin(), networkTypesToFilter.end(), NetworkType::OpenStr) != networkTypesToFilter.end()) ||
            networkTypesToFilter.empty())
        {
            networkNames.push_back(Common::ContainerEnvironment::ContainerNetworkName);
            networkNameList = (networkNameList.empty())
                ? (wformatString("{0}", Common::ContainerEnvironment::ContainerNetworkName))
                : (wformatString("{0},{1}", networkNameList, Common::ContainerEnvironment::ContainerNetworkName));
        }
    }

#if defined(PLATFORM_UNIX)
    if ((NetworkType & NetworkType::Enum::Isolated) != NetworkType::Enum::Isolated &&
        (NetworkType & NetworkType::Enum::Open) != NetworkType::Enum::Open)
    {
        if ((!networkTypesToFilter.empty() &&
            std::find(networkTypesToFilter.begin(), networkTypesToFilter.end(), NetworkType::OtherStr) != networkTypesToFilter.end()) ||
            networkTypesToFilter.empty())
        {
            networkNames.push_back(Common::ContainerEnvironment::ContainerNatNetworkTypeName);
            networkNameList = (networkNameList.empty())
                ? (wformatString("{0}", Common::ContainerEnvironment::ContainerNatNetworkTypeName))
                : (wformatString("{0},{1}", networkNameList, Common::ContainerEnvironment::ContainerNatNetworkTypeName));
        }
    }
#endif
}