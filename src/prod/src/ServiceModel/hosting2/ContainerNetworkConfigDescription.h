// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerNetworkConfigDescription : public Serialization::FabricSerializable
    {
    public:
        ContainerNetworkConfigDescription();

        ContainerNetworkConfigDescription(
            std::wstring const & openNetworkAssignedIp,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            std::map<std::wstring, std::wstring> const & portBindings,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            ServiceModel::NetworkType::Enum networkType);

        ContainerNetworkConfigDescription(ContainerNetworkConfigDescription const & other) = default;
        ContainerNetworkConfigDescription(ContainerNetworkConfigDescription && other) = default;

        ContainerNetworkConfigDescription & operator = (ContainerNetworkConfigDescription const & other) = default;
        ContainerNetworkConfigDescription & operator = (ContainerNetworkConfigDescription && other) = default;

        __declspec(property(get = get_OpenNetworkAssignedIp)) std::wstring const & OpenNetworkAssignedIp;
        inline std::wstring const & get_OpenNetworkAssignedIp() const { return openNetworkAssignedIp_; };

        __declspec(property(get = get_OverlayNetworkResources)) std::map<std::wstring, std::wstring> const & OverlayNetworkResources;
        inline std::map<std::wstring, std::wstring> const & get_OverlayNetworkResources() const { return overlayNetworkResources_; };

        __declspec(property(get = get_PortBindings)) std::map<std::wstring, std::wstring> const & PortBindings;
        inline std::map<std::wstring, std::wstring> const & get_PortBindings() const { return portBindings_; };

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; };

        __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
        inline std::wstring const & get_NodeName() const { return nodeName_; };

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        inline std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; };

        __declspec(property(get = get_NetworkType)) ServiceModel::NetworkType::Enum const & NetworkType;
        inline ServiceModel::NetworkType::Enum const & get_NetworkType() const { return networkType_; };

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;

        void GetNetworkNames(
            __in std::vector<std::wstring> & networkTypesToFilter,
            __out std::vector<std::wstring> & networkNames,
            __out std::wstring & networkNameList) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_NETWORK_CONFIG_DESCRIPTION & fabricContainerNetworkConfig) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(openNetworkAssignedIp_, overlayNetworkResources_, portBindings_, nodeId_, nodeName_, nodeIpAddress_, networkType_);
        
    private:
        std::wstring openNetworkAssignedIp_;
        std::map<std::wstring, std::wstring> overlayNetworkResources_;
        std::map<std::wstring, std::wstring> portBindings_;
        std::wstring nodeId_;
        std::wstring nodeName_;
        std::wstring nodeIpAddress_;
        ServiceModel::NetworkType::Enum networkType_;
    };
}
