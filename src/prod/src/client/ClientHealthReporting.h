// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientHealthReporting :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Client>,
        public Common::RootedObject,
        public Common::FabricComponent
    {
        DENY_COPY(ClientHealthReporting);
    public:
        ClientHealthReporting(
            Common::ComponentRoot const & root,
            __in FabricClientImpl &client,
            std::wstring const & traceContext);

        __declspec(property(get=get_ReportingComponent)) HealthReportingComponent & HealthReportingClient;
        HealthReportingComponent & get_ReportingComponent() const { return *(healthClient_); }
            
    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:

        class FabricClientHealthReportTransport : public HealthReportingTransport
        {
        public:
            FabricClientHealthReportTransport(
                Common::ComponentRoot const & root,
                __in FabricClientImpl &client);

            virtual Common::AsyncOperationSPtr BeginReport(
                Transport::MessageUPtr && message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndReport(
                Common::AsyncOperationSPtr const & operation,
                __out Transport::MessageUPtr & reply);

        private:
            FabricClientImpl &client_;
        };

        std::unique_ptr<FabricClientHealthReportTransport> healthReportTransportUPtr_;
        HealthReportingComponentSPtr healthClient_;
        FabricClientImpl &client_;
        std::wstring traceContext_;
    };
}
