// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class FabricApplicationGatewayManager
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>
        , public Naming::IGatewayManager
        , public std::enable_shared_from_this<FabricApplicationGatewayManager>
    {
    public:
        FabricApplicationGatewayManager(
            Common::FabricNodeConfigSPtr const & nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const & activatorClient,
            Common::ComponentRoot const & componentRoot)
            : RootedObject(componentRoot)
            , nodeConfig_(nodeConfig)
            , activatorClient_(activatorClient)
        {
        }

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

        Common::ErrorCode GetHostedServiceParameters(__out Hosting2::HostedServiceParameters &);
        Common::ErrorCode GetGatewayCertThumbprints(
            Common::Thumbprint const* loadedCredThumbprint,
            __out Common::Thumbprint & serverCertThumbprint);

        void SettingsUpdateHandler(std::weak_ptr<FabricApplicationGatewayManager> const &);
        Common::ErrorCode OnSettingsUpdated();
        Common::ErrorCode ActivateOnSettingsUpdate();

        void CreateCertMonitorTimerIfNeeded();
        void StopCertMonitorTimer();
        void CertMonitorCallback();

        Common::Thumbprint certThumbprint_;
        Common::FabricNodeConfigSPtr nodeConfig_;
        Hosting2::IFabricActivatorClientSPtr activatorClient_;

        Common::RwLock certMonitorTimerLock_;
        Common::TimerSPtr certMonitorTimer_;
    };
}
