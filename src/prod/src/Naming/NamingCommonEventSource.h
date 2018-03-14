// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{

#define NAMING_COMMON_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, NamingCommon, trace_id, trace_level, __VA_ARGS__)

    class NamePropertyOperation;

    class NamingCommonEventSource
    { 
    public:
        DECLARE_STRUCTURED_TRACE( ADFTrace, uint16, std::wstring, Common::ErrorCode, __int64);
        DECLARE_STRUCTURED_TRACE( NamingUriTrace, uint16, std::wstring);
        DECLARE_STRUCTURED_TRACE( CuidTrace, uint16, Common::Guid);
        DECLARE_STRUCTURED_TRACE( PSDTrace, uint16, Reliability::ServiceDescription, int, __int64, std::wstring, __int64, __int64, std::wstring);
        DECLARE_STRUCTURED_TRACE( HandlerIdTrace, uint16, LONGLONG);
        DECLARE_STRUCTURED_TRACE( UnknownEnumValue, uint16, int);
        DECLARE_STRUCTURED_TRACE( InvalidEnumValue, uint16);
                        
        NamingCommonEventSource()
            : NAMING_COMMON_STRUCTURED_TRACE( ADFTrace, 40, Info, "ADF(name={1},error={2},storeVersion={3})", "contextSequenceId", "name", "error", "storeVersion")
            , NAMING_COMMON_STRUCTURED_TRACE( NamingUriTrace, 50, Info, "'{1}' ", "contextSequenceId", "name")
            , NAMING_COMMON_STRUCTURED_TRACE( CuidTrace, 51, Info, "{1} ", "contextSequenceId", "cuid")
            , NAMING_COMMON_STRUCTURED_TRACE( PSDTrace, 52, Info, "{1} : ({2}, {3}) {4} - [{5},{6}] {7}", "contextSequenceId", "service", "partitionScheme", "version", "cuids", "lowRange", "highRange", "partitionNames")
            , NAMING_COMMON_STRUCTURED_TRACE( HandlerIdTrace, 54, Info, "{1};", "contextSequenceId", "handlerId")
            , NAMING_COMMON_STRUCTURED_TRACE( UnknownEnumValue, 60, Info, "Unknown value {1}", "contextSequenceId", "value")
            , NAMING_COMMON_STRUCTURED_TRACE( InvalidEnumValue, 61, Info, "-invalid-", "contextSequenceId")
        {
        }

        static Common::Global<NamingCommonEventSource> EventSource;
    };
}
