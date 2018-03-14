// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GatewayManagerWrapper
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>
        , public Naming::IGatewayManager
        , public std::enable_shared_from_this<GatewayManagerWrapper>
    {
    public:

        static IGatewayManagerSPtr CreateGatewayManager(
            Common::FabricNodeConfigSPtr const &nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const &activatorClient,
            Common::ComponentRoot const &root);

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
        GatewayManagerWrapper(
            Common::FabricNodeConfigSPtr const &nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const &activatorClient,
            Common::ComponentRoot const &root);

        bool IsApplicationGatewayEnabled(
                Common::FabricNodeConfigSPtr const &nodeConfig,
                Common::ComponentRoot const &componentRoot);

        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;

        IGatewayManagerSPtr fabricGatewaySPtr_;
        IGatewayManagerSPtr fabricApplicationGatewaySPtr_;
        std::wstring traceId_;
    };
}
