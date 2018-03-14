// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    #define NAMING_GATEWAY_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, NamingGateway, trace_id, trace_level, __VA_ARGS__)

    struct BroadcastRequestEntry;

    class GatewayEventSource
    { 
    public:        
        DECLARE_STRUCTURED_TRACE( GatewayOpenComplete, Federation::NodeInstance, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE( BroadcastEventManagerReceivedUpdate, std::wstring, Common::Uri, std::wstring);
        DECLARE_STRUCTURED_TRACE( BroadcastEventManagerAddRequest, std::wstring, BroadcastRequestEntry);
        DECLARE_STRUCTURED_TRACE( PsdCacheAdd, std::wstring, PartitionedServiceDescriptor, bool);
        DECLARE_STRUCTURED_TRACE( PsdCacheRemove, std::wstring, Common::Uri, bool);
        DECLARE_STRUCTURED_TRACE( StartStoreCommunication, std::wstring, Common::Uri);
        DECLARE_STRUCTURED_TRACE( RetryProcessRequest, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE( NotificationServiceNotFound, std::wstring, Common::Uri, __int64);
        DECLARE_STRUCTURED_TRACE( NotificationWaitBroadcast, std::wstring, std::vector<BroadcastRequestEntry>);
        DECLARE_STRUCTURED_TRACE( NotificationOnBroadcast, std::wstring, Common::Uri);
        DECLARE_STRUCTURED_TRACE( NotificationNoPsdAfterBroadcast, std::wstring, Common::Uri);
        DECLARE_STRUCTURED_TRACE( NotificationAddFailure, std::wstring, std::wstring, AddressDetectionFailure, std::wstring, __int64);
        DECLARE_STRUCTURED_TRACE( NotificationFailureNotNewer, std::wstring, AddressDetectionFailure, __int64);
        DECLARE_STRUCTURED_TRACE( NotificationAddRsp, std::wstring, ResolvedServicePartition, uint64);
        DECLARE_STRUCTURED_TRACE( NotificationProcessRequest, std::wstring, Common::Uri, std::wstring, Common::Guid, int64, int64, __int64, bool);
        DECLARE_STRUCTURED_TRACE( NotificationSendReply, std::wstring, uint64, uint64, uint64, uint64);
        DECLARE_STRUCTURED_TRACE( NotificationUnregisterBroadcast, std::wstring);
        DECLARE_STRUCTURED_TRACE( NotificationMsgSizeLimitRsp, std::wstring, ResolvedServicePartition, uint64, int);
        DECLARE_STRUCTURED_TRACE( NotificationMsgSizeLimitFailure, std::wstring, AddressDetectionFailure, uint64, int);

        // Get service description
        DECLARE_STRUCTURED_TRACE( TryRefreshPsdCache, std::wstring, Common::NamingUri);
        DECLARE_STRUCTURED_TRACE( TryGetPsdFromCache, std::wstring, Common::NamingUri);
        DECLARE_STRUCTURED_TRACE( ReadCachedPsd, std::wstring, PartitionedServiceDescriptor, int64);
        DECLARE_STRUCTURED_TRACE( RetryPsdCacheRead, std::wstring, Common::NamingUri);
        DECLARE_STRUCTURED_TRACE( PsdCacheReadFailed, std::wstring, Common::NamingUri, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE( FetchPsdFromFM, std::wstring, Common::NamingUri);
        DECLARE_STRUCTURED_TRACE( RetryFetchPsdFromFM, std::wstring, Common::NamingUri, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE( FetchPsdFromFMFailed, std::wstring, Common::NamingUri, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE( FetchPsdFromNaming, std::wstring, Common::NamingUri, bool);
        DECLARE_STRUCTURED_TRACE( UpdatedPsdCache, std::wstring, PartitionedServiceDescriptor, int64);
        DECLARE_STRUCTURED_TRACE( PsdNotFound, std::wstring, Common::NamingUri, int64);

        // Resolve service
        DECLARE_STRUCTURED_TRACE( ResolveFromCache, std::wstring, Common::Guid, int64, Reliability::GenerationNumber, int64, Reliability::GenerationNumber);
        DECLARE_STRUCTURED_TRACE( ResolveFromFM, std::wstring, Common::Guid, int64, Reliability::GenerationNumber);
        DECLARE_STRUCTURED_TRACE( RetryResolveFromFM, std::wstring, Common::TimeSpan, Common::ErrorCode);

        // Service location notification
        DECLARE_STRUCTURED_TRACE( NotificationInvalidatePsd, std::wstring, Common::NamingUri, int64, PartitionKey);
        DECLARE_STRUCTURED_TRACE( NotificationPsdFetchFailed, std::wstring, Common::NamingUri, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE( NotificationRspNotFound, std::wstring, Common::NamingUri, Common::Guid);
        DECLARE_STRUCTURED_TRACE( EntreeServiceIpcChannelRequestProcessing, Common::ActivityId, std::wstring, Common::TimeSpan);

        // Push notifications
        DECLARE_STRUCTURED_TRACE( PushNotificationProcessing, std::wstring, Common::ActivityId, Reliability::GenerationNumber, Common::VersionRangeCollection, int64, Common::VersionRangeCollection);
        DECLARE_STRUCTURED_TRACE( PushNotificationSkipped, std::wstring, Common::ActivityId, std::wstring);
        DECLARE_STRUCTURED_TRACE( PushNotificationMatched, std::wstring, Common::ActivityId, Common::NamingUri, int64, int64, Reliability::ServiceTableEntry);

        GatewayEventSource()
            : NAMING_GATEWAY_STRUCTURED_TRACE( GatewayOpenComplete, 4, Info, "{0}: Gateway has opened @ {1} with {2}", "gatewayTraceId", "listenAddress", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( BroadcastEventManagerReceivedUpdate, 10, Noise, "{0}: BroadcastEventManager received Name={1}, CUIDs={2}.", "gatewayTraceId", "serviceName", "cuid")
            , NAMING_GATEWAY_STRUCTURED_TRACE( BroadcastEventManagerAddRequest, 11, Noise, "{0}: Register broadcast updates for {1}", "gatewayTraceId", "request") 
            , NAMING_GATEWAY_STRUCTURED_TRACE( PsdCacheAdd, 20, Noise, "{0}: Store {1} success={2}.", "gatewayTraceId", "psd", "success")
            , NAMING_GATEWAY_STRUCTURED_TRACE( PsdCacheRemove, 21, Noise, "{0}: Remove {1} success={2}.", "gatewayTraceId", "name", "success")
            , NAMING_GATEWAY_STRUCTURED_TRACE( StartStoreCommunication, 30, Noise, "{0}: Start store communication for {1}.", "gatewayTraceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( RetryProcessRequest, 31, Noise, "{0}: Retry go to store for {1} names", "gatewayTraceId", "nameCount")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationServiceNotFound, 40, Info, "{0}: user service not found: name={1} version={2}.", "gatewayTraceId", "name", "storeVersion")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationWaitBroadcast, 41, Info, "{0}: Waiting for new broadcasts for {1}.", "gatewayTraceId", "requests")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationOnBroadcast, 42, Info, "{0}: {1} updated.", "gatewayTraceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationNoPsdAfterBroadcast, 43, Info, "{0}: {1}: psd doesn't exist after broadcast, retry.", "gatewayTraceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationAddFailure, 44, Noise, "{0}: {1}: Add {2}: previous error={3},ver={4}.", "gatewayTraceId", "name", "failure", "previousError", "previousVersion")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationFailureNotNewer, 45, Noise, "{0}: {1} is not newer: previous version={2}.", "gatewayTraceId", "failure", "previousVersion")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationAddRsp, 46, Noise, "{0}: Add {1}. MsgSize={2}", "gatewayTraceId", "rsp", "msgSize")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationProcessRequest, 48, Noise, "{0}: Request for {1}, previous={2}, CUID={3}, GatewayVersion(generation={4},FMVer={5},StoreVer={6}): has change={7}", "gatewayTraceId", "name", "previous", "cuid", "generation", "fmVer", "storeVer", "updated")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationSendReply, 49, Noise, "{0}: Send reply with {1} RSPs and {2} failures, msgSize={3}, index={4}", "gatewayTraceId", "rspCount", "failureCount", "msgSize", "index")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationUnregisterBroadcast, 50, Noise, "{0}: Removing broadcast requests", "gatewayTraceId")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationMsgSizeLimitRsp, 51, Info, "{0}: Can't add {1} because limit encountered, current size = {2}, count = {3}", "gatewayTraceId", "rsp", "msgSize", "replyCount")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationMsgSizeLimitFailure, 52, Info, "{0}: Can't add {1} because limit encountered, current size = {2}, count = {3}", "gatewayTraceId", "failure", "msgSize", "replyCount")


            // Get service description
            , NAMING_GATEWAY_STRUCTURED_TRACE( TryRefreshPsdCache, 59, Noise, "{0}: try refresh PSD cache: name={1}", "traceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( TryGetPsdFromCache, 60, Noise, "{0}: try get PSD from cache: name={1}", "traceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( ReadCachedPsd, 61, Noise, "{0}: read cached PSD: psd={1} version={2}", "traceId", "psd", "version")
            , NAMING_GATEWAY_STRUCTURED_TRACE( RetryPsdCacheRead, 62, Info, "{0}: retrying PSD cache read: name={1}", "traceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( PsdCacheReadFailed, 63, Info, "{0}: PSD cache read failed: name={1} error={2}", "traceId", "name", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( FetchPsdFromFM, 64, Info, "{0}: fetching PSD from FM: name={1}", "traceId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( RetryFetchPsdFromFM, 65, Info, "{0}: retrying PSD fetch from FM: name={1} error={2}", "traceId", "name", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( FetchPsdFromFMFailed, 66, Info, "{0}: failed to fetch PSD from FM: name={1} error={2}", "traceId", "name", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( FetchPsdFromNaming, 67, Info, "{0}: fetching PSD from Naming: name={1} prefix={2}", "traceId", "name", "prefix")
            , NAMING_GATEWAY_STRUCTURED_TRACE( UpdatedPsdCache, 68, Info, "{0}: updated PSD cache: psd={1} version={2}", "traceId", "psd", "version")
            , NAMING_GATEWAY_STRUCTURED_TRACE( PsdNotFound, 69, Info, "{0}: PSD not found: name={1} lsn={2}", "traceId", "name", "lsn")

            // Resolve service
            , NAMING_GATEWAY_STRUCTURED_TRACE( ResolveFromCache, 70, Noise, "{0}: cuid={1} client(ver={2} gen={3}) gateway(ver={4} gen={5})", "traceId", "cuid", "cversion", "cgeneration", "gversion", "ggeneration")
            , NAMING_GATEWAY_STRUCTURED_TRACE( ResolveFromFM, 71, Noise, "{0}: cuid={1} client(ver={2} gen={3})", "traceId", "cuid", "cversion", "cgeneration")
            , NAMING_GATEWAY_STRUCTURED_TRACE( RetryResolveFromFM, 72, Info, "{0}: retrying resolve from FM: delay={1} error={2}", "traceId", "delay", "error")

            // Service location notification
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationInvalidatePsd, 80, Info, "{0}: invalidating cached PSD: name={1} psd={2} key={3}", "traceId", "name", "version", "key")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationPsdFetchFailed, 81, Info, "{0}: failed to fetch PSD: name={1} error={2}", "traceId", "name", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( NotificationRspNotFound, 82, Info, "{0}: RSP entry not found: name={1} cuid={2}", "traceId", "name", "cuid")
            , NAMING_GATEWAY_STRUCTURED_TRACE( EntreeServiceIpcChannelRequestProcessing, 83, Info, "{0} Processing request, Action: {1}, RemainingTime {2}", "activityId", "action", "remainingTime")

            // Push notifications
            , NAMING_GATEWAY_STRUCTURED_TRACE( PushNotificationProcessing, 90, Info, "[{1}]: processing update: cache.gen={2} cache.vers={3} count={4} update.vers={5}", "id", "notificationId", "cgen", "cver", "partitions", "uvers")
            , NAMING_GATEWAY_STRUCTURED_TRACE( PushNotificationSkipped, 91, Info, "[{1}]: could not parse service name '{2}'", "id", "notificationId", "name")
            , NAMING_GATEWAY_STRUCTURED_TRACE( PushNotificationMatched, 92, Info, "[{1}]: matched update ({2}): exact={3} prefix={4} partition={5}", "id", "notificationId", "uri", "exactCount", "prefixCount", "partition")
        {
        }
    };
}
