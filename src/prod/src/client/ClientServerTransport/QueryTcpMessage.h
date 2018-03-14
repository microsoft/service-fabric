// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class QueryTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        static Common::GlobalWString QueryAction;

        QueryTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            std::wstring address = L"")
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            if (! address.empty())
            {
                Headers.Add(Query::QueryAddressHeader(address));    
            }
        }

        static Client::ClientServerRequestMessageUPtr GetQueryRequest(std::wstring const & queryName, ServiceModel::QueryArgumentMap const & queryArgs, std::wstring const & address)
        {
            return Common::make_unique<QueryTcpMessage>(QueryAction, Common::make_unique<Query::QueryRequestMessageBody>(queryName, queryArgs), address);
        }

        static Client::ClientServerRequestMessageUPtr GetQueryRequest(Query::QueryNames::Enum queryName, Query::QueryType::Enum queryType, ServiceModel::QueryArgumentMap const & queryArgs, std::wstring const & address)
        {
            return Common::make_unique<QueryTcpMessage>(QueryAction, Common::make_unique<Query::QueryRequestMessageBodyInternal>(queryName, queryType, queryArgs), address);
        }

        static Client::ClientServerRequestMessageUPtr GetQueryRequest(std::unique_ptr<Query::QueryRequestMessageBody> && body)
        {
            return Common::make_unique<QueryTcpMessage>(QueryAction, std::move(body));
        }

    private:

        static const Transport::Actor::Enum actor_;
    };
}
