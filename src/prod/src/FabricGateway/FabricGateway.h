// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "Common/Common.h"
#include "Federation/Federation.h"
#include "Hosting2/Hosting.h"
#include "Reliability/Reliability.h"
#include "Reliability/Failover/Failover.h"
#include "Naming/naming.h"
#include "httpgateway/HttpGateway.h"

namespace FabricGateway
{
    //
    // FabricGateway keeps track of the gateway's(tcp and http) for the windows fabric node.
    //
    class Gateway;
    typedef std::shared_ptr<Gateway> GatewaySPtr;

    class Gateway
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::FabricGateway>
    {
    public:

        static Common::ErrorCode Create(
            Common::FabricNodeConfigSPtr const &,
            std::wstring const &,
            __out GatewaySPtr &fabricGatewaySPtr);

        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            std::wstring const &,
            Common::FabricNodeConfigSPtr &&,
            __in Common::AsyncCallback const & callback);

        typedef Common::EventT<Common::ErrorCode> FailedEvent;
        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            std::wstring const &,
            FailedEvent::EventHandler const & failedEventHandler,
            __in Common::AsyncCallback const & callback);

        static Common::ErrorCode EndCreateAndOpen(
            __in Common::AsyncOperationSPtr const & operation, 
            __out GatewaySPtr & server);

        FailedEvent::HHandler RegisterFailedEvent(FailedEvent::EventHandler const & handler);
        bool UnRegisterFailedEvent(FailedEvent::HHandler hHandler);

    protected:

        //
        // AsyncFabricComponent methods.
        //

        Common::AsyncOperationSPtr OnBeginOpen(
            __in Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(__in Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            __in Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(__in Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:
        Gateway(Common::FabricNodeConfigSPtr const &config);

        class CreateAndOpenAsyncOperation;
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        Common::ErrorCode CreateSecuritySettings(__out Transport::SecuritySettings &);
        void EnableTransportHealthReporting();
        void ReportTransportHealth(
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & dynamicProperty,
            std::wstring const & property,
            Common::TimeSpan ttl);

        void RegisterForConfigUpdates();
        void SecuritySettingUpdateHandler(std::weak_ptr<Common::ComponentRoot> const &);
        Common::ErrorCode OnSecuritySettingsUpdated();

        void RecordError(Common::ErrorCode const & error);
        Common::ErrorCode GetError();

        void ReportCrlHealth(size_t cacheSize, std::wstring const & description); 

        std::unique_ptr<Naming::EntreeServiceProxy> entreeServiceProxy_;
        std::unique_ptr<Naming::ProxyToEntreeServiceIpcChannel> proxyIpcClient_;
        std::shared_ptr<HttpGateway::HttpGatewayImpl> httpGateway_;
        Common::FabricNodeConfigSPtr config_;

        Client::HealthReportingComponentSPtr healthClient_;
        Common::RwLock securitySettingsUpdateLock_;

        FailedEvent failedEvent_;
        Common::ErrorCode error_;
        Common::RwLock errorLock_;
    };
}

