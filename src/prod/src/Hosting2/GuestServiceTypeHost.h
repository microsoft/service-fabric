// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GuestServiceTypeHost :
        public IGuestServiceTypeHost,
        public Common::ComponentRoot,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(GuestServiceTypeHost)

    public:
        GuestServiceTypeHost(
            HostingSubsystemHolder const & hostingHolder,
            ApplicationHostContext const & hostContext,
            std::vector<GuestServiceTypeInfo> const & serviceTypesToHost,
            CodePackageContext const & codePackageContext,
            std::wstring const & runtimeServiceAddress,
            vector<wstring> && depdendentCodePackages,
            std::vector<ServiceModel::EndpointDescription> && endpointDescriptions);

        virtual ~GuestServiceTypeHost();
        
        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        inline std::wstring const & get_ServicePackageActivationId() const
        {
            return codePackageContext_.CodePackageInstanceId.ServicePackageInstanceId.PublicActivationId;
        }

        __declspec(property(get = get_HostContext)) ApplicationHostContext const & HostContext;
        ApplicationHostContext const & get_HostContext() override { return hostContext_; }

        __declspec(property(get = get_CodeContext)) CodePackageContext const & CodeContext;
        CodePackageContext const & get_CodeContext() override { return codePackageContext_; }

        __declspec(property(get = get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostContext_.HostId; }

        __declspec(property(get = get_RuntimeId)) std::wstring const & RuntimeId;
        inline std::wstring const & get_RuntimeId() const { return runtimeId_; }

        __declspec(property(get = get_Hosting)) HostingSubsystem & Hosting;
        inline HostingSubsystem & get_Hosting() const { return hostingHolder_.Value; }

        __declspec(property(get = get_ServiceTypes)) std::vector<GuestServiceTypeInfo> const & ServiceTypes;
        inline std::vector<GuestServiceTypeInfo> const & get_ServiceTypes() const { return serviceTypesToHost_; }

        __declspec(property(get = get_InProcAppHost)) InProcessApplicationHostSPtr const & InProcAppHost;
        inline InProcessApplicationHostSPtr const & get_InProcAppHost() const { return applicationhost_; }

        __declspec(property(get = get_DependentCodePackages)) std::vector<std::wstring> const & DependentCodePackages;
        std::vector<std::wstring> const & get_DependentCodePackages() override { return depdendentCodePackages_; }

        __declspec(property(get = get_ActivationManager)) Common::ComPointer<ComGuestServiceCodePackageActivationManager> & ActivationManager;
        Common::ComPointer<ComGuestServiceCodePackageActivationManager> & get_ActivationManager() override { return activationManager_; }

        __declspec(property(get = get_Endpoints)) std::vector<ServiceModel::EndpointDescription> const & Endpoints;
        std::vector<ServiceModel::EndpointDescription> const & get_Endpoints() override { return endpointDescriptions_; }

    public:
        void ProcessCodePackageEvent(CodePackageEventDescription eventDescription);

        Common::ErrorCode GetCodePackageActivator(
            _Out_ Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator) override;

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

    private:
        Common::ErrorCode InitializeApplicationHost();

    private:
        HostingSubsystemHolder const hostingHolder_;
        ApplicationHostContext const hostContext_;
        std::vector<GuestServiceTypeInfo> const serviceTypesToHost_;
        CodePackageContext const codePackageContext_;
        std::wstring const runtimeServiceAddress_;
        std::wstring runtimeId_;
        InProcessApplicationHostSPtr applicationhost_;
        Common::ComPointer<ComFabricRuntime> runtime_;
        Common::ComPointer<ComGuestServiceCodePackageActivationManager> activationManager_;
        std::vector<std::wstring> depdendentCodePackages_;
        std::vector<ServiceModel::EndpointDescription> endpointDescriptions_;
    };
}
