// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
#define BEGIN_NS_STRUCTURED_TRACES \
    public: \
    StoreServiceEventSource() : \

#define END_NS_STRUCTURED_TRACES \
    dummyMember() \
    { } \
    bool dummyMember; \

#define DECLARE_NS_STRUCTURED_TRACE(trace_name, ...) \
    DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define NS_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, NamingStoreService, trace_id, trace_level, __VA_ARGS__),

    class StoreServiceEventSource
    { 
        BEGIN_NS_STRUCTURED_TRACES

        NS_STRUCTURED_TRACE( StoreOpenComplete, 4, Info, "{0}: opened", "traceId" )
        // Name operations
        NS_STRUCTURED_TRACE( AOStartCreateName, 5, Noise, "{0}: create {1}, complete={2}", "traceId", "name", "tentative")
        NS_STRUCTURED_TRACE( AOInnerCreateNameRequestSendComplete, 6, Noise, "{0}: AO({1}:{2}) {3} request NO({4}:{5})", "traceId", "aoNodeId", "aoInstanceId", "name", "noNodeId", "noInstanceId" )
        NS_STRUCTURED_TRACE( AOInnerCreateNameReplyReceiveComplete, 8, Noise, "{0}: AO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AOCreateNameComplete, 10, Noise, "{0}: AO({1}:{2}) processed {3}, error={4}", "traceId", "nodeId", "instanceId", "name", "error" )
        NS_STRUCTURED_TRACE( NOInnerCreateNameRequestReceiveComplete, 12, Info, "{0}: NO({1}:{2}) received {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerCreateNameRequestProcessComplete, 14, Info, "{0}: NO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        
        NS_STRUCTURED_TRACE( AOStartDeleteName, 15, Noise, "{0}: delete {1}, complete={2}, explicit={3}", "traceId", "name", "tentative", "explicit")
        NS_STRUCTURED_TRACE( AOInnerDeleteNameRequestSendComplete, 16, Noise, "{0}: AO({1}:{2}) {3} request NO({4}:{5})", "traceId", "authoritOwneryNodeId", "aoInstanceId", "name", "noNodeId", "noInstanceId" )
        NS_STRUCTURED_TRACE( AOInnerDeleteNameReplyReceiveComplete, 18, Noise, "{0}: AO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AODeleteNameComplete, 20, Noise, "{0}: AO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteNameRequestReceiveComplete, 22, Noise, "{0}: NO({1}:{2}) received {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteNameRequestProcessComplete, 24, Noise, "{0}: NO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        // Service operations
        NS_STRUCTURED_TRACE( AOInnerCreateServiceRequestToNOSendComplete, 26, Noise, "{0}: AO({1}:{2}) request {3} NO({4}:{5})", "traceId", "aoNodeId", "aoInstanceId", "name", "noNodeId", "noInstanceId" )
        NS_STRUCTURED_TRACE( AOInnerUpdateServiceRequestToNOSendComplete, 27, Noise, "{0}: AO({1}:{2}) request {3} NO({4}:{5})", "traceId", "aoNodeId", "aoInstanceId", "name", "noNodeId", "noInstanceId" )
        NS_STRUCTURED_TRACE( AOInnerCreateServiceReplyFromNOReceiveComplete, 28, Noise, "{0}: AO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AOCreateServiceComplete, 30, Noise, "{0}: AO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AOUpdateServiceComplete, 31, Noise, "{0}: AO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerCreateServiceToFMSendComplete, 32, Noise, "{0}: NO({1}:{2}) {3} request FM", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerUpdateServiceToFMSendComplete, 33, Noise, "{0}: NO({1}:{2}) {3} request FM", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerCreateServiceReplyFromFMReceiveComplete, 34, Noise, "{0}: NO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerCreateServiceRequestReceiveComplete, 36, Noise, "{0}: NO({1}:{2}) received {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerUpdateServiceRequestReceiveComplete, 37, Noise, "{0}: NO({1}:{2}) received {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerCreateServiceRequestProcessComplete, 38, Noise, "{0}: NO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerUpdateServiceRequestProcessComplete, 39, Noise, "{0}: NO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AOInnerDeleteServiceRequestToNOSendComplete, 40, Noise, "{0}: AO({1}:{2}) {3} request NO({4}:{5})", "traceId", "aoNodeId", "aoInstanceId", "name", "noNodeId", "noInstanceId" )
        NS_STRUCTURED_TRACE( AOInnerDeleteServiceReplyFromNOReceiveComplete, 42, Noise, "{0}: AO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( AODeleteServiceComplete, 44, Noise, "{0}: AO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteServiceToFMSendComplete, 46, Noise, "{0}: NO({1}:{2}) {3} request FM", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteServiceReplyFromFMReceiveComplete, 48, Noise, "{0}: NO({1}:{2}) reply {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteServiceRequestReceiveComplete, 50, Noise, "{0}: NO({1}:{2}) received {3}", "traceId", "nodeId", "instanceId", "name" )
        NS_STRUCTURED_TRACE( NOInnerDeleteServiceRequestProcessComplete, 52, Noise, "{0}: NO({1}:{2}) processed {3}", "traceId", "nodeId", "instanceId", "name" )
        // Named locks
        NS_STRUCTURED_TRACE( AcquireNamedLockStart, 62, Noise, "{0} {1} ({2})", "traceId", "operation", "name" )
        NS_STRUCTURED_TRACE( AcquireNamedLockSuccess, 64, Noise, "{0} {1} ({2})", "traceId", "operation", "name" )
        NS_STRUCTURED_TRACE( AcquireNamedLockFailed, 66, Info, "{0} {1} ({2}) ({3})", "traceId", "operation", "name", "error" )
        NS_STRUCTURED_TRACE( ReleasedNamedLock, 68, Noise, "{0} {1}] ({2})", "traceId", "operation", "name" )
        NS_STRUCTURED_TRACE( DeleteNamedLock, 70, Noise, "{0} {1}", "traceId", "operation" )

        END_NS_STRUCTURED_TRACES

        DECLARE_NS_STRUCTURED_TRACE( StoreOpenComplete, std::wstring )
        // Name operations
        DECLARE_NS_STRUCTURED_TRACE( AOStartCreateName, std::wstring, std::wstring, bool )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerCreateNameRequestSendComplete, std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64 )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerCreateNameReplyReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AOCreateNameComplete, std::wstring, std::wstring, uint64, std::wstring, Common::ErrorCode )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateNameRequestReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateNameRequestProcessComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AOStartDeleteName, std::wstring, std::wstring, bool, bool )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerDeleteNameRequestSendComplete, std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64 )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerDeleteNameReplyReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AODeleteNameComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteNameRequestReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteNameRequestProcessComplete, std::wstring, std::wstring, uint64, std::wstring )
        // Service operations
        DECLARE_NS_STRUCTURED_TRACE( AOInnerCreateServiceRequestToNOSendComplete, std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64 )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerUpdateServiceRequestToNOSendComplete, std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64 )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerCreateServiceReplyFromNOReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AOCreateServiceComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AOUpdateServiceComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateServiceToFMSendComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerUpdateServiceToFMSendComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateServiceReplyFromFMReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateServiceRequestReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerUpdateServiceRequestReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerCreateServiceRequestProcessComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerUpdateServiceRequestProcessComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerDeleteServiceRequestToNOSendComplete, std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64 )
        DECLARE_NS_STRUCTURED_TRACE( AOInnerDeleteServiceReplyFromNOReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( AODeleteServiceComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteServiceToFMSendComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteServiceReplyFromFMReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteServiceRequestReceiveComplete, std::wstring, std::wstring, uint64, std::wstring )
        DECLARE_NS_STRUCTURED_TRACE( NOInnerDeleteServiceRequestProcessComplete, std::wstring, std::wstring, uint64, std::wstring )
        // Named locks
        DECLARE_NS_STRUCTURED_TRACE( AcquireNamedLockStart, std::wstring, int64, Common::Uri )
        DECLARE_NS_STRUCTURED_TRACE( AcquireNamedLockSuccess, std::wstring, int64, Common::Uri )
        DECLARE_NS_STRUCTURED_TRACE( AcquireNamedLockFailed, std::wstring, int64, Common::Uri, Common::ErrorCode )
        DECLARE_NS_STRUCTURED_TRACE( ReleasedNamedLock, std::wstring, int64, Common::Uri )
        DECLARE_NS_STRUCTURED_TRACE( DeleteNamedLock, std::wstring, int64 )
    };
}
