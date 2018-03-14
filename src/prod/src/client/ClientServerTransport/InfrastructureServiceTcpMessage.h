// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class InfrastructureServiceTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString RunCommandAction;
        static Common::GlobalWString InvokeInfrastructureCommandAction;
        static Common::GlobalWString InvokeInfrastructureQueryAction;
        static Common::GlobalWString ReportStartTaskSuccessAction;
        static Common::GlobalWString ReportFinishTaskSuccessAction;
        static Common::GlobalWString ReportTaskFailureAction;

        InfrastructureServiceTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            WrapForInfrastructureService();
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetRunCommand(std::unique_ptr<Management::InfrastructureService::RunCommandMessageBody> && body)
        {
            return Common::make_unique<InfrastructureServiceTcpMessage>(RunCommandAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetInvokeInfrastructureCommand(std::unique_ptr<Management::InfrastructureService::RunCommandMessageBody> && body)
        {
            return Common::make_unique<InfrastructureServiceTcpMessage>(InvokeInfrastructureCommandAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetInvokeInfrastructureQuery(std::unique_ptr<Management::InfrastructureService::RunCommandMessageBody> && body)
        { 
            return Common::make_unique<InfrastructureServiceTcpMessage>(InvokeInfrastructureQueryAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetReportStartTaskSuccess(std::unique_ptr<Management::InfrastructureService::ReportTaskMessageBody> && body)
        {
            return Common::make_unique<InfrastructureServiceTcpMessage>(ReportStartTaskSuccessAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetReportFinishTaskSuccess(std::unique_ptr<Management::InfrastructureService::ReportTaskMessageBody> && body)
        {
            return Common::make_unique<InfrastructureServiceTcpMessage>(ReportFinishTaskSuccessAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetReportTaskFailure(std::unique_ptr<Management::InfrastructureService::ReportTaskMessageBody> && body)
        { 
            return Common::make_unique<InfrastructureServiceTcpMessage>(ReportTaskFailureAction, std::move(body));
        }

    private:

        void WrapForInfrastructureService();

        static const Transport::Actor::Enum actor_;
    };
}
