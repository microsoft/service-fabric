// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    // base class for system services with mixed native/ managed layers, like FUOS, FAS, FIS, etc.
    class SystemServiceTcpMessageBase : public Client::ClientServerRequestMessage
    {
    protected:

        SystemServiceTcpMessageBase(
            std::wstring const & action,
            Transport::Actor::Enum const & actor,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : Client::ClientServerRequestMessage(action, actor, std::move(body))
        {
        }

        SystemServiceTcpMessageBase(
            std::wstring const & action,
            Transport::Actor::Enum const & actor)
            : Client::ClientServerRequestMessage(action, actor)
        {
        }
        
    public:

        template<class T>
        static Client::ClientServerRequestMessageUPtr GetSystemServiceMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::SystemServiceMessageBody> && body)
        {
            return Common::make_unique<T>(action, std::move(body));
        }

        template<class T>
        static Client::ClientServerRequestMessageUPtr GetSystemServiceMessage(
            std::wstring const & action)
        {
            return Common::make_unique<T>(action);
        }
    };
}
