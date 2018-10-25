// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class ClusterManagerTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString ProvisionApplicationTypeAction;
        static Common::GlobalWString CreateApplicationAction;
        static Common::GlobalWString UpdateApplicationAction;
        static Common::GlobalWString DeleteApplicationAction;
        static Common::GlobalWString DeleteApplicationAction2;
        static Common::GlobalWString UpgradeApplicationAction;
        static Common::GlobalWString UpdateApplicationUpgradeAction;
        static Common::GlobalWString UnprovisionApplicationTypeAction;
        static Common::GlobalWString CreateServiceAction;
        static Common::GlobalWString CreateServiceFromTemplateAction;
        static Common::GlobalWString DeleteServiceAction;
        static Common::GlobalWString GetUpgradeStatusAction;
        static Common::GlobalWString ReportUpgradeHealthAction;
        static Common::GlobalWString MoveNextUpgradeDomainAction;
        static Common::GlobalWString ProvisionFabricAction;
        static Common::GlobalWString UpgradeFabricAction;
        static Common::GlobalWString UpdateFabricUpgradeAction;
        static Common::GlobalWString GetFabricUpgradeStatusAction;
        static Common::GlobalWString ReportFabricUpgradeHealthAction;
        static Common::GlobalWString MoveNextFabricUpgradeDomainAction;
        static Common::GlobalWString UnprovisionFabricAction;
        static Common::GlobalWString StartInfrastructureTaskAction;
        static Common::GlobalWString FinishInfrastructureTaskAction;
        static Common::GlobalWString RollbackApplicationUpgradeAction;
        static Common::GlobalWString RollbackFabricUpgradeAction;

        static Common::GlobalWString CreateApplicationResourceAction;

        ClusterManagerTcpMessage(std::wstring const & action)
            : Client::ClientServerRequestMessage(action, actor_)
        {
        }

        ClusterManagerTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetProvisionApplicationType(
            std::unique_ptr<Management::ClusterManager::ProvisionApplicationTypeDescription> && body) 
        { 
            return Common::make_unique<ClusterManagerTcpMessage>(ProvisionApplicationTypeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateApplication(
            std::unique_ptr<Management::ClusterManager::CreateApplicationMessageBody> && body)
        { 
            return Common::make_unique<ClusterManagerTcpMessage>(CreateApplicationAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateApplication(
            std::unique_ptr<Management::ClusterManager::UpdateApplicationMessageBody> && body)
        {
            return Common::make_unique<ClusterManagerTcpMessage>(UpdateApplicationAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteApplication2(
            std::unique_ptr<Management::ClusterManager::DeleteApplicationMessageBody> && body)
        {
            return Common::make_unique<ClusterManagerTcpMessage>(DeleteApplicationAction2, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteApplication(
            std::unique_ptr<NamingUriMessageBody> && body)
        {
            return Common::make_unique<ClusterManagerTcpMessage>(DeleteApplicationAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpgradeApplication(
            std::unique_ptr<Management::ClusterManager::ApplicationUpgradeDescription> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UpgradeApplicationAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateApplicationUpgrade(
            std::unique_ptr<Management::ClusterManager::UpdateApplicationUpgradeMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UpdateApplicationUpgradeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUnprovisionApplicationType(
            std::unique_ptr<Management::ClusterManager::UnprovisionApplicationTypeDescription> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UnprovisionApplicationTypeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateService(
            std::unique_ptr<Management::ClusterManager::CreateServiceMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(CreateServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateServiceFromTemplate(
            std::unique_ptr<Management::ClusterManager::CreateServiceFromTemplateMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(CreateServiceFromTemplateAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteService(
            std::unique_ptr<Management::ClusterManager::DeleteServiceMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(DeleteServiceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetUpgradeStatus(
            std::unique_ptr<NamingUriMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(GetUpgradeStatusAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetReportUpgradeHealth(
            std::unique_ptr<Management::ClusterManager::ReportUpgradeHealthMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(ReportUpgradeHealthAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetMoveNextUpgradeDomain(
            std::unique_ptr<Management::ClusterManager::MoveNextUpgradeDomainMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(MoveNextUpgradeDomainAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetProvisionFabric(
            std::unique_ptr<Management::ClusterManager::ProvisionFabricBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(ProvisionFabricAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpgradeFabric(
            std::unique_ptr<Management::ClusterManager::FabricUpgradeDescription> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UpgradeFabricAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateFabricUpgrade(
            std::unique_ptr<Management::ClusterManager::UpdateFabricUpgradeMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UpdateFabricUpgradeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetGetFabricUpgradeStatus()
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(GetFabricUpgradeStatusAction);
        }

        static Client::ClientServerRequestMessageUPtr GetReportFabricUpgradeHealth(
            std::unique_ptr<Management::ClusterManager::ReportUpgradeHealthMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(ReportFabricUpgradeHealthAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetMoveNextFabricUpgradeDomain(
            std::unique_ptr<Management::ClusterManager::MoveNextUpgradeDomainMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(MoveNextFabricUpgradeDomainAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUnprovisionFabric(
            std::unique_ptr<FabricVersionMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(UnprovisionFabricAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetStartInfrastructureTask(
            std::unique_ptr<Management::ClusterManager::InfrastructureTaskDescription> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(StartInfrastructureTaskAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetFinishInfrastructureTask(
            std::unique_ptr<Management::ClusterManager::TaskInstance> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(FinishInfrastructureTaskAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRollbackApplicationUpgrade(
            std::unique_ptr<Management::ClusterManager::RollbackApplicationUpgradeMessageBody> && body)
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(RollbackApplicationUpgradeAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetRollbackFabricUpgrade()
        {  
            return Common::make_unique<ClusterManagerTcpMessage>(RollbackFabricUpgradeAction);
        }

        static Client::ClientServerRequestMessageUPtr GetCreateApplicationResource(
            std::unique_ptr<Management::ClusterManager::CreateApplicationResourceMessageBody> && body)
        {
            return Common::make_unique<ClusterManagerTcpMessage>(CreateApplicationResourceAction, std::move(body));
        }

    private:

        static const Transport::Actor::Enum actor_;
    };
}
