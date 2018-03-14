// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Naming
{
    class TestGatewayManager
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::FabricActivator>
        , public Naming::IGatewayManager
        , public std::enable_shared_from_this<TestGatewayManager>
    {
    public:

        static IGatewayManagerSPtr CreateGatewayManager(
            Common::FabricNodeConfigSPtr const &nodeConfig,
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

        TestGatewayManager(
            Common::FabricNodeConfigSPtr const & nodeConfig,
            Common::ComponentRoot const & componentRoot);

        Common::FabricNodeConfigSPtr nodeConfig_;
        std::wstring ipcListenAddress_;
        std::shared_ptr<FabricGateway::Gateway> fabricGatewaySPtr_;
    };
}
