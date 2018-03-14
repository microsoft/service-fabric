// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GuestServiceTypeHostManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(GuestServiceTypeHostManager)

    public:
        GuestServiceTypeHostManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);

        virtual ~GuestServiceTypeHostManager();

        Common::AsyncOperationSPtr BeginOpenGuestServiceTypeHost(
            std::wstring const & hostId,
            std::vector<std::wstring> const & typesToHost,
            CodePackageContext const & codePackageContext,
            std::vector<ServiceModel::EndpointDescription> && endpointDescriptions,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndOpenGuestServiceTypeHost(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginCloseGuestServiceTypeHost(
            std::wstring const & hostId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCloseGuestServiceTypeHost(
            Common::AsyncOperationSPtr const & operation);

        void AbortGuestServiceTypeHost(std::wstring const & hostId);

        Common::ErrorCode Test_HasGuestServiceTypeHost(std::wstring const & hostId, __out bool & isPresent);
        Common::ErrorCode Test_GetRuntimeId(std::wstring const & hostId, __out std::wstring & runtimeId);
        Common::ErrorCode Test_GetTypeHostCount(__out size_t & typeHostCount);

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        virtual void OnAbort() override;

    private:
        void NotifyAppHostHostManager(std::wstring const & hostId, uint exitCode);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        class OpenGuestServiceTypeHostAsyncOperation;
        class CloseGuestServiceTypeHostAsyncOperation;

        class GuestServiceTypeHostMap;
        typedef std::unique_ptr<GuestServiceTypeHostMap> GuestServiceTypeHostMapUPtr;

    private:
        HostingSubsystem & hosting_;
        GuestServiceTypeHostMapUPtr typeHostMap_;
    };
}
