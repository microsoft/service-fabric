// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GuestServiceTypeHost :
        public Common::ComponentRoot,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(GuestServiceTypeHost)

    public:
        GuestServiceTypeHost(
            HostingSubsystemHolder const & hostingHolder,
            std::wstring const & hostId,
            std::vector<std::wstring> const & serviceTypesToHost,
            CodePackageContext const & codePackageContext,
            std::wstring const & runtimeServiceAddress,
            std::vector<ServiceModel::EndpointDescription> && endpointDescriptions);

        virtual ~GuestServiceTypeHost();
        
        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        inline std::wstring const & get_ServicePackageActivationId() const
        {
            return codePackageContext_.CodePackageInstanceId.ServicePackageInstanceId.PublicActivationId;
        }

        __declspec(property(get = get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; }

        __declspec(property(get = get_RuntimeId)) std::wstring const & RuntimeId;
        inline std::wstring const & get_RuntimeId() const { return runtimeId_; }

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
        std::wstring const hostId_;
        HostingSubsystemHolder const hostingHolder_;
        std::vector<std::wstring> const serviceTypesToHost_;
        CodePackageContext const codePackageContext_;
        std::wstring const runtimeServiceAddress_;
        std::vector<ServiceModel::EndpointDescription> endpointDescriptions_;

        std::wstring runtimeId_;
        ApplicationHostSPtr applicationhost_;
        Common::ComPointer<ComFabricRuntime> runtime_;
    };
}
