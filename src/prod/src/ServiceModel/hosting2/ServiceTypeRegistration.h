// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceTypeRegistration;
    typedef std::shared_ptr<ServiceTypeRegistration> ServiceTypeRegistrationSPtr;

    class ServiceTypeRegistration
    {
        DENY_COPY(ServiceTypeRegistration)

    public:

        // public static constructor for test only use
        static ServiceTypeRegistrationSPtr Test_Create(
            ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
            ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackagePublicActivationId,
            std::wstring const & hostId,
            std::wstring const & runtimeId,
            std::wstring const & codePackageName,
            int64 servicePackageInstanceId,
            DWORD pid);

        ServiceTypeRegistration(
            ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
            ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackagePublicActivationId,
            std::wstring const & hostId,
            DWORD hostProcessId,
            std::wstring const & runtimeId,
            std::wstring const & codePackageName,
            int64 servicePackageInstanceId);

        ~ServiceTypeRegistration();

        __declspec(property(get=get_VersionedServiceTypeId)) ServiceModel::VersionedServiceTypeIdentifier const & VersionedServiceTypeId;
        ServiceModel::VersionedServiceTypeIdentifier const & get_VersionedServiceTypeId() const;

        __declspec(property(get=get_ServiceTypeId)) ServiceModel::ServiceTypeIdentifier const & ServiceTypeId;
        ServiceModel::ServiceTypeIdentifier const & get_ServiceTypeId() const;

        __declspec(property(get=get_ServicePackageVersionInstance)) ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance;
        ServiceModel::ServicePackageVersionInstance const & get_ServicePackageVersionInstance() const;

        __declspec(property(get = get_ActivationContext)) ServiceModel::ServicePackageActivationContext const &  ActivationContext;
        ServiceModel::ServicePackageActivationContext const &  get_ActivationContext() const { return activationContext_; }

        __declspec(property(get=get_ServicePackageInstanceId)) int64 const & ServicePackageInstanceId;
        int64 const & get_ServicePackageInstanceId() const;

        __declspec(property(get=get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() const;

		__declspec(property(get = get_ServicePackagePublicActivationId)) std::wstring const & ServicePackagePublicActivationId;
		std::wstring const & get_ServicePackagePublicActivationId() const;

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        std::wstring const & get_HostId() const;

        __declspec(property(get=get_HostProcessId)) DWORD HostProcessId;
        DWORD get_HostProcessId() const;

        __declspec(property(get=get_RuntimeId)) std::wstring const & RuntimeId;
        std::wstring const & get_RuntimeId() const;        

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        ServiceModel::VersionedServiceTypeIdentifier const versionedServiceTypeId_;
        ServiceModel::ServicePackageActivationContext const activationContext_;
        std::wstring const hostId_;
        DWORD const hostProcessId_;
        std::wstring const runtimeId_;
        std::wstring const codePackageName_;
        int64 const servicePackageInstanceId_;
		std::wstring const servicePackagePublicActivationId_;
    };
}
