// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class IpcHealthReportingClient :
        public Common::RootedObject,
        public Common::FabricComponent
    {
          
    public:

        IpcHealthReportingClient(
            Common::ComponentRoot const & root,
            Transport::IpcClient & ipcClient,
            bool enableMaxNumberOfReportThrottle,
            std::wstring const & traceContext,
            Transport::Actor::Enum actor,
            bool isEnabled);

        ~IpcHealthReportingClient();
        Common::ErrorCode AddReport(
            ServiceModel::HealthReport && healthReport,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
        Common::ErrorCode AddReports(
            std::vector<ServiceModel::HealthReport> && healthReports,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
        //
        // FabricComponent methods
        //
    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();
        
       
    private:

        class IpcHealthReportingClientTransport : public Client::HealthReportingTransport
        {
        public:
            IpcHealthReportingClientTransport(
                Common::ComponentRoot const & root,
                IpcHealthReportingClient & owner);

            virtual Common::AsyncOperationSPtr BeginReport(
                Transport::MessageUPtr && message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndReport(
                Common::AsyncOperationSPtr const & operation,
                __out Transport::MessageUPtr & reply);

        private:
            IpcHealthReportingClient const & owner_;
        };
        Transport::IpcClient & ipcClient_;
        std::unique_ptr<IpcHealthReportingClientTransport> healthClientTransport_;
        Client::HealthReportingComponentSPtr healthClient_;
        bool enableMaxNumberOfReportThrottle;
        std::wstring traceContext_;
        Transport::Actor::Enum actor_;
        bool isEnabled_;
    };
 }

