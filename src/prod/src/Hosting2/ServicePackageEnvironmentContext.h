// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServicePackageInstanceEnvironmentContext : 
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ServicePackageInstanceEnvironmentContext)

    public:
        ServicePackageInstanceEnvironmentContext(ServicePackageInstanceIdentifier const & servicePackageInstanceId);
        ~ServicePackageInstanceEnvironmentContext();

        void AddNetworks(ServiceModel::NetworkType::Enum networkType, std::vector<std::wstring> networkNames);
        void GetNetworks(ServiceModel::NetworkType::Enum networkType, std::vector<std::wstring> & networkNames);
        bool NetworkExists(ServiceModel::NetworkType::Enum networkType);
        void AddEndpoint(EndpointResourceSPtr && endpointResource);
        void AddEtwProviderGuid(std::wstring const & etwProviderGuid);
        void AddAssignedIpAddresses(
            std::wstring const & codePackageName,
            std::wstring const & ipAddress);
        void AddAssignedOverlayNetworkResources(
            std::wstring const & codePackageName,
            std::map<std::wstring, std::wstring> & overlayNetworkResources);
        void AddCertificatePaths(std::map<std::wstring, std::wstring> const & certificatePaths, std::map<std::wstring, std::wstring> const & certificatePasswordPaths);
        void AddGroupContainerName(std::wstring const & containerName);
        void GetGroupAssignedNetworkResource(
            std::wstring & openNetworkIpAddress,
            std::map<std::wstring, std::wstring> & overlayNetworkResources,
            ServiceModel::NetworkType::Enum & networkType,
            std::wstring & networkResourceList);
#if defined(PLATFORM_UNIX)
        void SetContainerGroupIsolatedFlag(bool isIsolated);
#endif
        void SetContainerGroupSetupFlag(bool isEnabled);
        void Reset();

        __declspec(property(get=get_Endpoints)) std::vector<EndpointResourceSPtr> const & Endpoints;
        std::vector<EndpointResourceSPtr> const & get_Endpoints() const { return this->endpointResources_; }        

        __declspec(property(get=get_EtwProviderGuids)) std::vector<std::wstring> const & EtwProviderGuids;
        std::vector<std::wstring> const & get_EtwProviderGuids() const { return this->etwProviderGuids_; }        

        __declspec(property(get = get_GroupContainerName)) std::wstring const & GroupContainerName;
        std::wstring const & get_GroupContainerName() const { return this->groupContainerName_; }

        __declspec(property(get = get_Id)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        ServicePackageInstanceIdentifier const & get_Id() { return this->servicePackageInstanceId_; }

        __declspec(property(get = get_HasIpsAssigned)) bool HasIpsAssigned;
        bool get_HasIpsAssigned() { return !this->ipAddresses_.empty(); }

        __declspec(property(get = get_HasOverlayNetworkResourcesAssigned)) bool HasOverlayNetworkResourcesAssigned;
        bool get_HasOverlayNetworkResourcesAssigned() { return !this->overlayNetworkResources_.empty(); }

        __declspec(property(get = get_SetupContainerGroup)) bool SetupContainerGroup;
        bool get_SetupContainerGroup() { return this->setupContainerGroup_; }

#if defined(PLATFORM_UNIX)
        __declspec(property(get = get_ContainerGroupIsolated)) bool ContainerGroupIsolated;
        bool get_ContainerGroupIsolated() { return this->containerGroupIsolated_; }
#endif

        __declspec(property(get = get_CertificatePaths)) std::map<std::wstring, std::wstring> const & CertificatePaths;
        std::map<std::wstring, std::wstring> const & get_CertificatePaths() { return this->certificatePaths_; }

        __declspec(property(get = get_CertificatePasswordPaths)) std::map<std::wstring, std::wstring> const & CertificatePasswordPaths;
        std::map<std::wstring, std::wstring> const & get_CertificatePasswordPaths() { return this->certificatePasswordPaths_; }

        __declspec(property(get = get_FirewallPorts, put = set_FirewallPorts)) std::vector<LONG> FirewallPorts;
        std::vector<LONG> get_FirewallPorts() const { return this->firewallPorts_; }
        void set_FirewallPorts(std::vector<LONG> const& firewallPorts) { this->firewallPorts_ = firewallPorts; }

        Common::ErrorCode GetIpAddressAssignedToCodePackage(std::wstring const & codePackageName, __out std::wstring & ipAddress);

        Common::ErrorCode GetNetworkResourceAssignedToCodePackage(
            std::wstring const & codePackageName,
            __out std::wstring & openNetworkIpAddress,
            __out std::map<std::wstring, std::wstring> & overlayNetworkResources);

        void GetCodePackageOverlayNetworkNames(__out std::map<wstring, vector<wstring>> & codePackageNetworkNames);

    private:
        std::vector<EndpointResourceSPtr> endpointResources_;    
        std::vector<std::wstring> etwProviderGuids_;
        std::map<std::wstring, std::wstring> ipAddresses_;
        std::map<std::wstring, std::map<std::wstring, std::wstring>> overlayNetworkResources_;
        std::map<ServiceModel::NetworkType::Enum, std::vector<std::wstring>> associatedNetworks_;
        ServicePackageInstanceIdentifier servicePackageInstanceId_;
        std::map<std::wstring, std::wstring> certificatePaths_;
        std::map<std::wstring, std::wstring> certificatePasswordPaths_;
        std::wstring groupContainerName_;
#if defined(PLATFORM_UNIX)
        bool containerGroupIsolated_;
#endif
        bool setupContainerGroup_;
        // On succesful cleanup the ports are set to empty. Firewall cleanup during abort will be a no op.
        std::vector<LONG> firewallPorts_;
    };
}
