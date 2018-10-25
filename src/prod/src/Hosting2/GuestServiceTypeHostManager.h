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
            ApplicationHostContext const & appHostContext,
            std::vector<GuestServiceTypeInfo> const & typesToHost,
            CodePackageContext const & codePackageContext,
            std::vector<std::wstring> && depdendentCodePackages,
            std::vector<ServiceModel::EndpointDescription> && endpointDescriptions,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndOpenGuestServiceTypeHost(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginCloseGuestServiceTypeHost(
            ApplicationHostContext const & appHostContext,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCloseGuestServiceTypeHost(
            Common::AsyncOperationSPtr const & operation);

        void AbortGuestServiceTypeHost(ApplicationHostContext const & appHostContext);

        void ProcessCodePackageEvent(
            ApplicationHostContext const & appHostContext,
            CodePackageEventDescription eventDescription);

        Common::ErrorCode Test_HasGuestServiceTypeHost(std::wstring const & hostId, __out bool & isPresent);
        Common::ErrorCode Test_GetTypeHost(std::wstring const & hostId, __out GuestServiceTypeHostSPtr & guestTypeHost);
        Common::ErrorCode Test_GetTypeHostCount(__out size_t & typeHostCount);
        Common::ErrorCode Test_GetTypeHost(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            _Out_ GuestServiceTypeHostSPtr & guestTypeHost);

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
