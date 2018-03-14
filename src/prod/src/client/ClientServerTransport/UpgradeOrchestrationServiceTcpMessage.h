// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class UpgradeOrchestrationServiceTcpMessage : public SystemServiceTcpMessageBase
    {
    public:

        // Request actions
        static Common::GlobalWString StartClusterConfigurationUpgradeAction;
        static Common::GlobalWString GetClusterConfigurationUpgradeStatusAction;
        static Common::GlobalWString GetClusterConfigurationAction;
        static Common::GlobalWString GetUpgradesPendingApprovalAction;
        static Common::GlobalWString StartApprovedUpgradesAction;
        static Common::GlobalWString GetUpgradeOrchestrationServiceStateAction;
		static Common::GlobalWString SetUpgradeOrchestrationServiceStateAction;

        UpgradeOrchestrationServiceTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : SystemServiceTcpMessageBase(action, actor_, std::move(body))
        {
            WrapForUpgradeOrchestrationService();
        }

        UpgradeOrchestrationServiceTcpMessage(std::wstring const & action)
            : SystemServiceTcpMessageBase(action, actor_)
        {
            WrapForUpgradeOrchestrationService();
        }

        // Requests: keep for backward compatibility purpose only

        static Client::ClientServerRequestMessageUPtr StartClusterConfigurationUpgrade(std::unique_ptr<Management::UpgradeOrchestrationService::StartUpgradeMessageBody> && body)
        {
            return Common::make_unique<UpgradeOrchestrationServiceTcpMessage>(StartClusterConfigurationUpgradeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetClusterConfigurationUpgradeStatus()
        {
            return Common::make_unique<UpgradeOrchestrationServiceTcpMessage>(GetClusterConfigurationUpgradeStatusAction);
        }

        static Client::ClientServerRequestMessageUPtr GetClusterConfiguration(std::unique_ptr<Management::UpgradeOrchestrationService::GetClusterConfigurationMessageBody> && body)
        {
            return Common::make_unique<UpgradeOrchestrationServiceTcpMessage>(GetClusterConfigurationAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetUpgradesPendingApproval()
        {
            return Common::make_unique<UpgradeOrchestrationServiceTcpMessage>(GetUpgradesPendingApprovalAction);
        }

        static Client::ClientServerRequestMessageUPtr GetStartApprovedUpgrades()
        {
            return Common::make_unique<UpgradeOrchestrationServiceTcpMessage>(StartApprovedUpgradesAction);
        }

    private:

        void WrapForUpgradeOrchestrationService();

        static const Transport::Actor::Enum actor_;
    };
}
