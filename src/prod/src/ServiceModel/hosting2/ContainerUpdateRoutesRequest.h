// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerUpdateRoutesRequest : public Serialization::FabricSerializable
    {
    public:
        ContainerUpdateRoutesRequest();
        ContainerUpdateRoutesRequest(
            std::wstring const & containerId,
            std::wstring const & containerName,
            std::wstring const & applicationId,
            std::wstring const & applicationName,
            ServiceModel::NetworkType::Enum networkType,
            std::vector<std::wstring> const & gatewayIpAddresses,
            bool autoremove,
            bool isContainerRoot,
            std::wstring const & cgroupName,
            int64 timeoutTicks);
            
        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        __declspec(property(get = get_ContainerId)) wstring const & ContainerId;
        wstring const & get_ContainerId() const { return containerId_; }

        __declspec(property(get = get_ContainerName)) wstring const & ContainerName;
        wstring const & get_ContainerName() const { return containerName_; }

        __declspec(property(get = get_ApplicationId)) wstring const & ApplicationId;
        wstring const & get_ApplicationId() const { return applicationId_; }

        __declspec(property(get = get_ApplicationName)) wstring const & ApplicationName;
        wstring const & get_ApplicationName() const { return applicationName_; }
        
        __declspec(property(get = get_NetworkType)) ServiceModel::NetworkType::Enum NetworkType;
        ServiceModel::NetworkType::Enum get_NetworkType() const { return networkType_; }

        __declspec(property(get = get_GatewayIpAddresses)) std::vector<std::wstring> const & GatewayIpAddresses;
        std::vector<std::wstring> const & get_GatewayIpAddresses() const { return gatewayIpAddresses_; }

        __declspec(property(get = get_AutoRemove)) bool AutoRemove;
        bool get_AutoRemove() const { return autoRemove_; }

        __declspec(property(get = get_IsContainerRoot)) bool IsContainerRoot;
        bool get_IsContainerRoot() const { return isContainerRoot_; }

        __declspec(property(get = get_CgroupName)) wstring const & CgroupName;
        wstring const & get_CgroupName() const { return cgroupName_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        
        FABRIC_FIELDS_10(containerId_, containerName_, applicationId_, applicationName_, networkType_, 
            gatewayIpAddresses_, autoRemove_, isContainerRoot_, cgroupName_, timeoutTicks_);

    private:
        std::wstring containerId_;
        std::wstring containerName_;
        std::wstring applicationId_;
        std::wstring applicationName_;
        ServiceModel::NetworkType::Enum networkType_;
        std::vector<std::wstring> gatewayIpAddresses_;
        int64 timeoutTicks_;
        bool autoRemove_;
        bool isContainerRoot_;
        std::wstring cgroupName_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::ContainerUpdateRoutesRequest);
