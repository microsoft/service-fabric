// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Naming
{
    class FabricGatewayManager 
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>
        , public Naming::IGatewayManager
        , public std::enable_shared_from_this<FabricGatewayManager>
    {
    public:

        FabricGatewayManager(
            Common::FabricNodeConfigSPtr const & nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const & activatorClient,
            Common::ComponentRoot const & componentRoot);

        virtual bool RegisterGatewaySettingsUpdateHandler();

        virtual Common::AsyncOperationSPtr BeginActivateGateway(
            std::wstring const &gatewayIpcServerListenAddress,
            Common::TimeSpan const&,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndActivateGateway(
            Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr BeginDeactivateGateway(
            Common::TimeSpan const&,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndDeactivateGateway(
            Common::AsyncOperationSPtr const &);

        virtual void AbortGateway();

    private:

        void SettingsUpdateHandler(std::weak_ptr<FabricGatewayManager> const &);
        Common::ErrorCode OnSettingsUpdated();
        Common::ErrorCode GetServerCertThumbprint_CallerHoldingWLock(
            _In_opt_ Common::Thumbprint const * loadedCredThumbprint,
            _Out_ Common::Thumbprint & serverCertThumbprint);

        Common::ErrorCode ActivateOnSettingsUpdate_CallerHoldingWLock();

        void CreateCertMonitorTimerIfNeeded();
        void StopCertMonitorTimer();
        void CertMonitorCallback();

        Common::AsyncOperationSPtr UpdateHandler(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode GetHostedServiceParameters_CallerHoldingWLock(__out Hosting2::HostedServiceParameters &);

        Common::FabricNodeConfigSPtr nodeConfig_;
        Common::EventHandler handler_;
        Hosting2::IFabricActivatorClientSPtr activatorClient_;
        Common::RwLock settingsUpdateLock_;
        std::wstring ipcListenAddress_;
        Common::Thumbprint serverCertThumbprint_;

        Common::TimerSPtr certMonitorTimer_;
    };
}
