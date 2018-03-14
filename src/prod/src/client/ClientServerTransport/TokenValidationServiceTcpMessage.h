// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class TokenValidationServiceTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString GetMetadataAction;
        static Common::GlobalWString ValidateTokenAction;

    private:

        TokenValidationServiceTcpMessage(std::wstring const & action)
            : Client::ClientServerRequestMessage(action, actor_)
        {
            WrapForTokenValidationService();
        }

        TokenValidationServiceTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            WrapForTokenValidationService();
        }

    public:

        // Requests
        static Client::ClientServerRequestMessageUPtr GetMetadata()
        {
            return std::unique_ptr<TokenValidationServiceTcpMessage>(new TokenValidationServiceTcpMessage(GetMetadataAction));
        }

        static Client::ClientServerRequestMessageUPtr GetValidateToken(
            Management::TokenValidationService::ValidateTokenMessageBody const & body)
        {
            return std::unique_ptr<TokenValidationServiceTcpMessage>(new TokenValidationServiceTcpMessage(
                ValidateTokenAction, 
                Common::make_unique<Management::TokenValidationService::ValidateTokenMessageBody>(body)));
        }

    private:

        void WrapForTokenValidationService();

        static const Transport::Actor::Enum actor_;
    };
}
