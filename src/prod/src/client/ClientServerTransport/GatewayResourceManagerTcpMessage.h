// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class GatewayResourceManagerTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString CreateGatewayResourceAction;
        static Common::GlobalWString DeleteGatewayResourceAction;

    private:

        GatewayResourceManagerTcpMessage(std::wstring const & action)
            : Client::ClientServerRequestMessage(action, actor_)
        {
            WrapForGatewayResourceManager();
        }

        GatewayResourceManagerTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            WrapForGatewayResourceManager();
        }

    public:

        // Requests
        static Client::ClientServerRequestMessageUPtr GetCreateGatewayResource(
            std::unique_ptr<Management::GatewayResourceManager::CreateGatewayResourceMessageBody> && body)
        {
            return std::unique_ptr<GatewayResourceManagerTcpMessage>(new GatewayResourceManagerTcpMessage(
                CreateGatewayResourceAction,
                std::move(body)));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteGatewayResource(
            std::unique_ptr<Management::GatewayResourceManager::DeleteGatewayResourceMessageBody> && body)
        {
            return std::unique_ptr<GatewayResourceManagerTcpMessage>(new GatewayResourceManagerTcpMessage(
                DeleteGatewayResourceAction,
                std::move(body)));
        }

    private:

        void WrapForGatewayResourceManager();

        static const Transport::Actor::Enum actor_;
    };
}
