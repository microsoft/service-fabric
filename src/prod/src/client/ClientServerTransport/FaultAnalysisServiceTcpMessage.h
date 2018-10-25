// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class FaultAnalysisServiceTcpMessage : public SystemServiceTcpMessageBase
    {
    public:

        // Request actions
        static Common::GlobalWString StartPartitionDataLossAction;
        static Common::GlobalWString GetPartitionDataLossProgressAction;
        static Common::GlobalWString StartPartitionQuorumLossAction;
        static Common::GlobalWString GetPartitionQuorumLossProgressAction;
        static Common::GlobalWString StartPartitionRestartAction;
        static Common::GlobalWString GetPartitionRestartProgressAction;
        static Common::GlobalWString CancelTestCommandAction;
        static Common::GlobalWString StartChaosAction;
        static Common::GlobalWString StopChaosAction;
        static Common::GlobalWString GetChaosAction;
        static Common::GlobalWString GetChaosReportAction;
        static Common::GlobalWString GetChaosEventsAction;
        static Common::GlobalWString GetChaosScheduleAction;
        static Common::GlobalWString PostChaosScheduleAction;
        static Common::GlobalWString StartNodeTransitionAction;
        static Common::GlobalWString GetNodeTransitionProgressAction;

        FaultAnalysisServiceTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : SystemServiceTcpMessageBase(action, actor_, std::move(body))
        {
            WrapForFaultAnalysisService();
        }

        FaultAnalysisServiceTcpMessage(
            std::wstring const & action)
            : SystemServiceTcpMessageBase(action, actor_)
        {
            WrapForFaultAnalysisService();
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetStartPartitionDataLoss(std::unique_ptr<Management::FaultAnalysisService::InvokeDataLossMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StartPartitionDataLossAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetPartitionDataLossProgress(std::unique_ptr<Management::FaultAnalysisService::GetInvokeDataLossProgressMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetPartitionDataLossProgressAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartPartitionQuorumLoss(std::unique_ptr<Management::FaultAnalysisService::InvokeQuorumLossMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StartPartitionQuorumLossAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetPartitionQuorumLossProgress(std::unique_ptr<Management::FaultAnalysisService::GetInvokeQuorumLossProgressMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetPartitionQuorumLossProgressAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartPartitionRestart(std::unique_ptr<Management::FaultAnalysisService::RestartPartitionMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StartPartitionRestartAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetPartitionRestartProgress(std::unique_ptr<Management::FaultAnalysisService::GetRestartPartitionProgressMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetPartitionRestartProgressAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCancelTestCommand(std::unique_ptr<Management::FaultAnalysisService::CancelTestCommandMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(CancelTestCommandAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartChaos(std::unique_ptr<Management::FaultAnalysisService::StartChaosMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StartChaosAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStopChaos(std::unique_ptr<Management::FaultAnalysisService::StopChaosMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StopChaosAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetChaosReport(std::unique_ptr<Management::FaultAnalysisService::GetChaosReportMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetChaosReportAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetChaosEvents(std::unique_ptr<Management::FaultAnalysisService::GetChaosEventsMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetChaosEventsAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartNodeTransition(std::unique_ptr<Management::FaultAnalysisService::StartNodeTransitionMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(StartNodeTransitionAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetNodeTransitionProgress(std::unique_ptr<Management::FaultAnalysisService::GetNodeTransitionProgressMessageBody> && body)
        {
            return Common::make_unique<FaultAnalysisServiceTcpMessage>(GetNodeTransitionProgressAction, std::move(body));
        }

    private:

        void WrapForFaultAnalysisService();

        static const Transport::Actor::Enum actor_;
    };
}
