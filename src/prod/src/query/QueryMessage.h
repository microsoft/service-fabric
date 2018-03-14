// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryMessage
    {
    public:
        static Common::GlobalWString QueryReplyAction;
        static Common::GlobalWString QueryFailureAction;

        static Transport::MessageUPtr CreateMessage(std::wstring const & queryName, ServiceModel::QueryArgumentMap const & queryArgs, std::wstring const & address);
        static Transport::MessageUPtr CreateMessage(Query::QueryNames::Enum queryName, Query::QueryType::Enum queryType, ServiceModel::QueryArgumentMap const & queryArgs, std::wstring const & address);

        static Transport::MessageUPtr GetQueryReply(ServiceModel::QueryResult const & queryResult);

        static Transport::MessageUPtr GetQueryFailure(ServiceModel::QueryResult const & queryResult);

    private:
        static void AddHeaders(Transport::Message & message, std::wstring const & address);
    };
}
