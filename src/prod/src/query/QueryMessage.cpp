// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Query;
using namespace ServiceModel;
using namespace ClientServerTransport;

GlobalWString QueryMessage::QueryReplyAction = make_global<wstring>(L"QueryReply");
GlobalWString QueryMessage::QueryFailureAction = make_global<wstring>(L"QueryFailure");

Transport::MessageUPtr QueryMessage::CreateMessage(std::wstring const & queryName, QueryArgumentMap const & queryArgs, std::wstring const & address)
{
    QueryRequestMessageBody messageBody(queryName, queryArgs);
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(messageBody));
    AddHeaders(*message, address);
    return std::move(message);
}

Transport::MessageUPtr QueryMessage::CreateMessage(Query::QueryNames::Enum queryName, Query::QueryType::Enum queryType, QueryArgumentMap const & queryArgs, std::wstring const & address)
{
    QueryRequestMessageBodyInternal messageBody(queryName, queryType, queryArgs);
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(messageBody));
    AddHeaders(*message, address);
    return std::move(message);
}

void QueryMessage::AddHeaders(Transport::Message & message, std::wstring const & address) 
{
    message.Headers.Add(QueryAddressHeader(address));    
    message.Headers.Add(ActionHeader(QueryTcpMessage::QueryAction));
}

MessageUPtr QueryMessage::GetQueryReply(ServiceModel::QueryResult const & body)
{
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
    message->Headers.Add(ActionHeader(QueryReplyAction));    
    message->Headers.Add(ActorHeader(Actor::NamingGateway));    
    message->Headers.Add(FabricActivityHeader(Guid::NewGuid()));
    return std::move(message);
}

MessageUPtr QueryMessage::GetQueryFailure(ServiceModel::QueryResult const & body)
{
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
    message->Headers.Add(ActionHeader(QueryFailureAction));

    return std::move(message);
}
