// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
#define QUERY_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, Query, trace_id, trace_level, __VA_ARGS__)

    class QueryEventSource
    {
        public:
            DECLARE_STRUCTURED_TRACE( QueryForward, std::wstring, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE( QueryProcessHandler, Common::ActivityId, std::wstring, QueryNames::Trace);
            DECLARE_STRUCTURED_TRACE( QueryProcessGateway, Common::ActivityId, std::wstring, ServiceModel::QueryArgumentMap);
            DECLARE_STRUCTURED_TRACE( QueryNotFound, Common::ActivityId, std::wstring);
            DECLARE_STRUCTURED_TRACE( QueryArgsMissing, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE( QueryUnknownArgument, std::wstring, std::wstring);
            DECLARE_STRUCTURED_TRACE( ProcessingParallelQuery, Common::ActivityId, std::wstring, QueryNames::Trace);
            DECLARE_STRUCTURED_TRACE( QueryUnknownArgumentSet, std::wstring);
            DECLARE_STRUCTURED_TRACE( ProcessingSequentialQuery, Common::ActivityId, std::wstring, QueryNames::Trace, LONGLONG, QueryNames::Trace);

            QueryEventSource()
                : QUERY_STRUCTURED_TRACE( QueryForward, 5, Noise, "QueryMessageHandler at '{0}' forwarding query to handler at '{1}' with metadata '{2}'", "sourceQueryHandler", "destiationQueryHandler", "destinationQueryHandlerMetadata")
                , QUERY_STRUCTURED_TRACE( QueryProcessHandler, 6, Noise, "{0}: QueryMessageHandler at '{1}' is processing query: '{2}'", "activityId", "queryMessageHandler", "queryName")
                , QUERY_STRUCTURED_TRACE( QueryProcessGateway, 7, Info, "{0}: Processing incoming query: '{1}' with arguments: {2} ", "activityId", "queryName", "queryArguments")
                , QUERY_STRUCTURED_TRACE( QueryNotFound, 8, Warning, "{0}: Query '{1}' not found.", "activityId", "queryName")
                , QUERY_STRUCTURED_TRACE( QueryArgsMissing, 9, Warning, "Query arg '{0}' missing for query: '{1}'. ", "argumentName", "queryName")
                , QUERY_STRUCTURED_TRACE( QueryUnknownArgument, 10, Warning, "Unknown query argument '{0}' for query: '{1}'. ", "argumentName", "queryName")
                , QUERY_STRUCTURED_TRACE( ProcessingParallelQuery, 11, Noise, "{0}: QueryMessageHandler at '{1}' is processing parallel query: '{2}'", "activityId", "queryMessageHandler", "queryName")
                , QUERY_STRUCTURED_TRACE( QueryUnknownArgumentSet, 12, Warning, "Query argument set can not be determined for query: '{0}'", "queryName")
                , QUERY_STRUCTURED_TRACE(ProcessingSequentialQuery, 13, Noise, "{0}: QueryMessageHandler at '{1}' is processing sequential query: '{2}', Inner Query Number = {3}, Inner Query Name = {4}", "activityId", "queryMessageHandler", "queryName", "queryNumber", "innerQueryName")
            {
            }

            static Common::Global<QueryEventSource> EventSource;
    };
}
