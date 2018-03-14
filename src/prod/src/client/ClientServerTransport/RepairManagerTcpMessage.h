// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class RepairManagerTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString CreateRepairRequestAction;
        static Common::GlobalWString CancelRepairRequestAction;
        static Common::GlobalWString ForceApproveRepairAction;
        static Common::GlobalWString DeleteRepairRequestAction;
        static Common::GlobalWString UpdateRepairExecutionStateAction;
        static Common::GlobalWString UpdateRepairTaskHealthPolicyAction;        
            
        // Reply actions
        static Common::GlobalWString OperationSuccessAction;

        RepairManagerTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body = nullptr,
            bool isRoutedRequest = false)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            if (isRoutedRequest)
            {
                WrapForRepairManager();
            }
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetCreateRepairRequest(std::unique_ptr<Management::RepairManager::UpdateRepairTaskMessageBody> && body) 
        { 
            return Common::make_unique<RepairManagerTcpMessage>(CreateRepairRequestAction, std::move(body), true);
        }

        static Client::ClientServerRequestMessageUPtr GetCancelRepairRequest(std::unique_ptr<Management::RepairManager::CancelRepairTaskMessageBody> && body) 
        {
            return Common::make_unique<RepairManagerTcpMessage>(CancelRepairRequestAction, std::move(body), true);
        }

        static Client::ClientServerRequestMessageUPtr GetForceApproveRepair(std::unique_ptr<Management::RepairManager::ApproveRepairTaskMessageBody> && body) 
        { 
            return Common::make_unique<RepairManagerTcpMessage>(ForceApproveRepairAction, std::move(body), true);
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteRepairRequest(std::unique_ptr<Management::RepairManager::DeleteRepairTaskMessageBody> && body) 
        { 
            return Common::make_unique<RepairManagerTcpMessage>(DeleteRepairRequestAction, std::move(body), true);
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateRepairExecutionState(std::unique_ptr<Management::RepairManager::UpdateRepairTaskMessageBody> && body) 
        { 
            return Common::make_unique<RepairManagerTcpMessage>(UpdateRepairExecutionStateAction, std::move(body), true);
        }

        static Client::ClientServerRequestMessageUPtr GetUpdateRepairTaskHealthPolicy(std::unique_ptr<Management::RepairManager::UpdateRepairTaskHealthPolicyMessageBody> && body) 
        { 
            return Common::make_unique<RepairManagerTcpMessage>(UpdateRepairTaskHealthPolicyAction, std::move(body), true);
        }        

        // Replies
        static Client::ClientServerRequestMessageUPtr GetClientOperationSuccess() 
        {
            return Common::make_unique<RepairManagerTcpMessage>(OperationSuccessAction);
        }

        template <class TBody>
        static Client::ClientServerRequestMessageUPtr GetClientOperationSuccess(std::unique_ptr<TBody> && body)
        {
            return Common::make_unique<RepairManagerTcpMessage>(OperationSuccessAction, std::move(body));
        }

    private:

        void WrapForRepairManager();

        static const Transport::Actor::Enum actor_;
    };
}
