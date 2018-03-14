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

        void AddEndpoint(EndpointResourceSPtr && endpointResource);
        void AddEtwProviderGuid(std::wstring const & etwProviderGuid);
        void AddAssignedIpAddresses(
            std::wstring const & codePackageName,
            std::wstring const & ipAddress);
        void AddCertificatePaths(std::map<std::wstring, std::wstring> const & certificatePaths, std::map<std::wstring, std::wstring> const & certificatePasswordPaths);
        void AddGroupContainerName(std::wstring const & containerName);
        std::wstring GetGroupAssignedIp();
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

        __declspec(property(get = get_SetupContainerGroup)) bool SetupContainerGroup;
        bool get_SetupContainerGroup() { return this->setupContainerGroup_; }
        
        __declspec(property(get = get_CertificatePaths)) std::map<std::wstring, std::wstring> const & CertificatePaths;
        std::map<std::wstring, std::wstring> const & get_CertificatePaths() { return this->certificatePaths_; }

        __declspec(property(get = get_CertificatePasswordPaths)) std::map<std::wstring, std::wstring> const & CertificatePasswordPaths;
        std::map<std::wstring, std::wstring> const & get_CertificatePasswordPaths() { return this->certificatePasswordPaths_; }

        Common::ErrorCode GetIpAddressAssignedToCodePackage(std::wstring const & codePackageName, __out std::wstring & ipAddress);

    private:
        std::vector<EndpointResourceSPtr> endpointResources_;    
        std::vector<std::wstring> etwProviderGuids_;
        std::map<std::wstring, std::wstring> ipAddresses_;
        ServicePackageInstanceIdentifier servicePackageInstanceId_;
        std::map<std::wstring, std::wstring> certificatePaths_;
        std::map<std::wstring, std::wstring> certificatePasswordPaths_;
        std::wstring groupContainerName_;
        bool setupContainerGroup_;
    };
}
