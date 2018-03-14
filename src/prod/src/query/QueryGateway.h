// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Query
{
    class QueryGateway
        : protected Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
        DENY_COPY(QueryGateway)

    public:
        explicit QueryGateway(__in Query::QueryMessageHandler & messageHandler);
        
        Common::AsyncOperationSPtr BeginProcessIncomingQuery(
            Transport::Message & gatewayRequestMessage,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr parent);
        Common::ErrorCode EndProcessIncomingQuery(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Transport::MessageUPtr & replyMessage);

        ServiceModel::QueryResult GetQueryList();

        static bool TryGetQueryName(Transport::Message & gatewayRequestMessage, __out std::wstring &queryName);

    private:
        class ProcessIncomingQueryOperation;
        friend class ProcessIncomingQueryOperation;

        QueryMessageHandler & messageHandler_;
        std::map<std::wstring, Query::QueryNames::Enum, Common::IsLessCaseInsensitiveComparer<std::wstring>> nameEnumlookupMap_;
    };

    typedef std::unique_ptr<QueryGateway> QueryGatewayUPtr;
}
