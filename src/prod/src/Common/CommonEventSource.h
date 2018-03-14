// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    #define DECLARE_COMMON_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define COMMON_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::Common, \
            trace_id, \
            #trace_name "_" trace_type, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class CommonEventSource
    { 
    public:
        
        // An string trace
        // Simple mechanism to trace a constant string when you need to
        DECLARE_COMMON_STRUCTURED_TRACE(TraceConstantString,
            uint16,
            Common::StringLiteral
            );

        DECLARE_COMMON_STRUCTURED_TRACE(TraceDynamicWString,
            uint16,
            std::wstring
            );
        
        DECLARE_COMMON_STRUCTURED_TRACE(TraceDynamicString,
            uint16,
            std::string
            );

        DECLARE_COMMON_STRUCTURED_TRACE(ErrorCode,
            uint16,
            std::wstring
            );
         DECLARE_COMMON_STRUCTURED_TRACE(UriTrace,
            uint16,
            std::wstring, //scheme
            std::wstring, //path
            std::wstring, //querySeparator
            std::wstring, //query
            std::wstring, //fragmentSeparator
            std::wstring  //fragment
            );
        DECLARE_COMMON_STRUCTURED_TRACE(UriAuthorityTrace,
            uint16,
            std::wstring, //scheme
            std::wstring, //authority
            std::wstring, //path
            std::wstring, //querySeparator
            std::wstring, //query
            std::wstring, //fragmentSeparator
            std::wstring  //fragment
            );
        DECLARE_COMMON_STRUCTURED_TRACE(UnrecognizedUriTrace,
            uint16,
            int, //type
            std::wstring, //scheme
            std::wstring, //path
            std::wstring, //querySeparator
            std::wstring, //query
            std::wstring, //fragmentSeparator
            std::wstring  //fragment
            );
        DECLARE_COMMON_STRUCTURED_TRACE(TraceLruCacheEviction,
            std::wstring,
            std::wstring,
            size_t,
            size_t,
            size_t
            );
        DECLARE_COMMON_STRUCTURED_TRACE(JobQueueCtor,
            std::wstring,
            int,
            int,
            int
            );
        DECLARE_COMMON_STRUCTURED_TRACE(JobQueueEnterProcess,
            std::wstring,
            int
            );
        DECLARE_COMMON_STRUCTURED_TRACE(JobQueueLeaveProcess,
            std::wstring,
            int
            );
        DECLARE_COMMON_STRUCTURED_TRACE(JobQueueScheduleThread,
            std::wstring
            );
        DECLARE_COMMON_STRUCTURED_TRACE(JobQueueEnqueueFailed,
            std::wstring,
            uint64,
            Common::ErrorCode,
            int,
            int,
            int,
            int,
            uint64,
            uint64,
            uint64
            );
        DECLARE_COMMON_STRUCTURED_TRACE(VersionRange,
            uint16,
            int64,
            int64);

        CommonEventSource() :
            COMMON_STRUCTURED_TRACE(ErrorCode, 10, "ErrorCode", Info, "{1}", "contextSequenceId", "error"),
            COMMON_STRUCTURED_TRACE(UriTrace, 20, "UriTrace", Info, "{1}:{2}{3}{4}{5}{6}", "contextSequenceId", "scheme", "path", "querySeparator", "query", "fragmentSeparator", "fragment"),
            COMMON_STRUCTURED_TRACE(UriAuthorityTrace, 21, "UriAuthorityTrace", Info, "{1}://{2}{3}{4}{5}{6}{7}", "contextSequenceId", "scheme", "authority", "path", "querySeparator", "query", "fragmentSeparator", "fragment"),
            COMMON_STRUCTURED_TRACE(UnrecognizedUriTrace, 22, "UnrecognizedUriTrace", Info, "UNRECOGNIZED-URI-TYPE='{1}' {2}:{3}{4}{5}{6}{7}", "contextSequenceId", "type", "scheme", "path", "querySeparator", "query", "fragmentSeparator", "fragment"),
            COMMON_STRUCTURED_TRACE(TraceConstantString, 25, "String", Info, " {1} ", "contextSequenceId", "text"),
            COMMON_STRUCTURED_TRACE(TraceDynamicString, 26, "String", Info, " {1} ", "contextSequenceId", "text"),
            COMMON_STRUCTURED_TRACE(TraceDynamicWString, 27, "String", Info, " {1} ", "contextSequenceId", "text"),
            COMMON_STRUCTURED_TRACE(TraceLruCacheEviction, 30, "LruCacheEviction", Info, "Inserted={0} Evicted={1} Limit/Hash/List={2}/{3}/{4}", "newKey", "oldKey", "limit", "hashSize", "listSize"),
            COMMON_STRUCTURED_TRACE(JobQueueCtor, 40, "JobQueue", Info, "Queue created: MaxThreads={1}, MaxQueueSize={2}, MaxParallelWork={3}", "id", "maxThreads", "maxQueueSize", "maxParallelWork" ),
            COMMON_STRUCTURED_TRACE(JobQueueEnterProcess, 41, "JobQueue", Info, "Entering Process(), thread {1}", "id", "crtThread"),
            COMMON_STRUCTURED_TRACE(JobQueueLeaveProcess, 42, "JobQueue", Info, "Leaving Process(), thread {1}", "id", "crtThread"),
            COMMON_STRUCTURED_TRACE(JobQueueScheduleThread, 43, "JobQueue", Info, "Schedule thread", "id"),
            COMMON_STRUCTURED_TRACE(JobQueueEnqueueFailed, 44, "JobQueue", Info, "Enqueue work with sequence number {1} failed: {2}. Configuration: maxThreads={3},maxSize={4},maxPending={5}, activeThreads={6}, count: items={7},pending={8},asyncReady={9}", "id", "sn", "error", "maxThreads", "maxSize", "maxPending", "activeThreads", "items", "pending", "asyncReady"),

            COMMON_STRUCTURED_TRACE(VersionRange, 50, "VersionRange", Info, "[{1}, {2})", "contextSequenceId", "low", "high")
        {
        }

    public:
        static Common::Global<CommonEventSource> Events;
    };
}
