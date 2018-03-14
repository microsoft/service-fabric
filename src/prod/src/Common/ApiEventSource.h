// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_API_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define API_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::Api, \
                trace_id, \
                #trace_name "_" trace_type, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

#define API_STRUCTURED_TRACE_QUERY(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::Api, \
                trace_id, \
                trace_type, \
                Common::LogLevel::trace_level, \
                Common::TraceChannelType::Debug, \
                TRACE_KEYWORDS2(Default, ForQuery), \
                __VA_ARGS__)


namespace Common
{
    class ApiEventSource
    {
    public:
        DECLARE_API_STRUCTURED_TRACE(Start,
            Common::Guid,
            ApiMonitoring::ApiNameDescription,
            int64);

        DECLARE_API_STRUCTURED_TRACE(Slow,
            Common::Guid,
            ApiMonitoring::ApiNameDescription,
            int64,
			Common::DateTime);

        DECLARE_API_STRUCTURED_TRACE(Finish,
            Common::Guid,
            ApiMonitoring::ApiNameDescription,
            int64,
            double, /* milliseconds taken */
            Common::ErrorCode,
            std::wstring);

        DECLARE_API_STRUCTURED_TRACE(FinishWithServiceType,
            Common::Guid,
            ApiMonitoring::ApiNameDescription,
            int64,
            std::wstring, // service type name
            double, /* milliseconds taken */
            Common::ErrorCode,
            std::wstring);

        ApiEventSource() :
            API_STRUCTURED_TRACE(Start,                             4,      "Start",            Info,       "Start {1} on partition {0} replica {2}", "id", "api", "replicaId"),
            API_STRUCTURED_TRACE(Slow,                              5,      "Slow",             Warning,    "Api {1} slow on partition {0} replica {2}, StartTimeUTC = {3}", "id", "api", "replicaId", "startTime"),
            API_STRUCTURED_TRACE_QUERY(Finish,                      6,      "_Api_Finish",      Info,       "Finish {1} on partition {0} replica {2}. Elapsed = {3}ms. Error = {4}. Message = {5}", "id", "api", "replicaId", "elapsed", "error", "message"),
            API_STRUCTURED_TRACE_QUERY(FinishWithServiceType,       7,      "_Api_ST_Finish",   Info,       "Finish {1} on partition {0} replica {2}. ServiceType = {3}. Elapsed = {4}ms. Error = {5}. Message = {6}", "id", "api", "replicaId", "serviceType", "elapsed", "error", "message")
        {}

        static Common::Global<ApiEventSource> Events;
    };
}


