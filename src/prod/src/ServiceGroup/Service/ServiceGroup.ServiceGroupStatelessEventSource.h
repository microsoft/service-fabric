// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPSTATELESS_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupStateless, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupStatelessEventSource
    {
    public:
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_5_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_6_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_7_StatelessCompositeOpenCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupFactory, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupFactory, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupFactory, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroup, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroup, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroup, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroup, uintptr_t, int64, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroup, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroup, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_5_StatelessServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportFault, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_6_StatelessServiceGroup, std::wstring, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_7_StatelessServiceGroup, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_8_StatelessServiceGroup, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_9_StatelessServiceGroup, std::wstring, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_10_StatelessServiceGroup, std::wstring, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupOpen, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupOpen, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Warning_0_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupClose, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupAbort, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroupMember, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportMoveCost, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportInstanceHealth, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportPartitionHealth, std::wstring, uintptr_t, int64);

        ServiceGroupStatelessEventSource() :
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessCompositeOpenCloseOperationAsync, 4, Info, "this={0} partitionId={1} - Starting open", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessCompositeOpenCloseOperationAsync, 5, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting open failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessCompositeOpenCloseOperationAsync, 6, Info, "this={0} partitionId={1} - Starting close", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessCompositeOpenCloseOperationAsync, 7, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting close failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessCompositeOpenCloseOperationAsync, 8, Info, "this={0} partitionId={1} - Open completed with error code {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessCompositeOpenCloseOperationAsync, 9, Info, "this={0} partitionId={1} - Invoking callback", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessCompositeOpenCloseOperationAsync, 10, Info, "this={0} partitionId={1} - Finishing open", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessCompositeOpenCloseOperationAsync, 11, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing open failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_5_StatelessCompositeOpenCloseOperationAsync, 12, Info, "this={0} partitionId={1} - Finishing open success", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_6_StatelessCompositeOpenCloseOperationAsync, 13, Info, "this={0} partitionId={1} - Finishing close", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessCompositeOpenCloseOperationAsync, 14, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing close failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_7_StatelessCompositeOpenCloseOperationAsync, 15, Info, "this={0} partitionId={1} - Finishing close success", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupFactory, 16, Error, "this={0} - Could not allocate stateless service group", "object"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupFactory, 17, Error, "this={0} - Instance {1} deserialization failed with {2}", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupFactory, 18, Error, "this={0} - Instance {1} initialization failed with {2}", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupFactory, 19, Info, "this={0} - Instance {1} initialization succeeded", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroupFactory, 20, Error, "this={0} - No available service factories", "object"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroupFactory, 21, Error, "this={0} - Could not store service factories", "object"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroup, 22, Info, "this={0} instanceId={1} - Starting initialization", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroup, 23, Error, "this={0} instanceId={1} - Could not allocate stateless service member for service type {2}", "object", "instanceId", "name"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroup, 24, Error, "this={0} instanceId={1} - Could not find registered stateless service factory for service type {2}", "object", "instanceId", "name"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroup, 25, Error, "this={0} instanceId={1} - Stateless service member {2} initialization failed with {3}", "object", "instanceId", "partitionId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroup, 26, Info, "this={0} instanceId={1} - Stateless service member {2} initialization succeeded", "object", "instanceId", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroup, 27, Error, "this={0} instanceId={1} - Could not store stateless service member {2}", "object", "instanceId", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroup, 28, Info, "this={0} instanceId={1} - Stateless service member {2} stored successfully", "object", "instanceId", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroup, 29, Error, "this={0} instanceId={1} - Initialization failed with {2}", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroup, 30, Info, "this={0} instanceId={1} - Initialization succeeded", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroup, 31, Info, "this={1} instanceId={2} - Uninitialization", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_5_StatelessServiceGroup, 32, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} - System stateless service partition unavailable", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportFault, 33, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} -System stateless service partition unavailable", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_6_StatelessServiceGroup, 34, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Service member name is invalid", "id", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_7_StatelessServiceGroup, 35, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Service member name {3} is not a valid URI", "id", "object", "partitionId", "name"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_8_StatelessServiceGroup, 36, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Service member {3} not found", "id", "object", "partitionId", "name"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_9_StatelessServiceGroup, 37, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Could not resolve service member", "id", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_10_StatelessServiceGroup, 38, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - System stateless service partition unavailable", "id", "object", "partitionId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupOpen, 39, Error, Common::TraceChannelType::Debug, "this={0} instanceId={1} - IFabricStatelessServicePartition::GetPartitionInfo failed with {2}", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupOpen, 40, Error, "this={0} instanceId={1} - Could not store service instance name", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Warning_0_StatelessServiceGroupOpen, 41, Warning, "this={1} instanceId={2} - Cleaning up failed open", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupOpen, 42, Info, "this={1} instanceId={2} - Completing open", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupOpen, 43, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} - Open failed with {3}", "id", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_3_StatelessServiceGroupOpen, 44, Error, "this={1} instanceId={2} - Failed to retrieve composite service endpoint with {3}", "id", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupOpen, 45, Info, "this={1} instanceId={2} - End open success", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupOpen, 46, Info, "this={1} instanceId={2} - Returning service endpoint", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupClose, 47, Info, "this={1} instanceId={2} - Completing close", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupClose, 48, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} - Close failed with {3}", "id", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupClose, 49, Info, "this={1} instanceId={2} - End close success", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupAbort, 50, Info, "this={1} instanceId={2} - Aborting members", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupOpen, 51, Info, "this={1} instanceId={2} - Starting open", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_4_StatelessServiceGroupOpen, 52, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} - Begin open failed with {3}", "id", "object", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroupOpen, 53, Info, "this={1} instanceId={2} - Begin open success", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupClose, 54, Info, "this={1} instanceId={2} - Starting close", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupClose, 55, Info, "this={1} instanceId={2} - Begin close success", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_0_StatelessServiceGroupMember, 56, Info, "this={0} partitionId={1} instanceId={2} - Initialization started", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessServiceGroupMember, 57, Error, "this={0} partitionId={1} instanceId={2} - SizeTToULong conversion failed with {3}", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_1_StatelessServiceGroupMember, 58, Error, "this={0} partitionId={1} instanceId={2} - Could not store service name", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_2_StatelessServiceGroupMember, 59, Error, "this={0} partitionId={1} instanceId={2} - CreateInstance failed with {3}", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_1_StatelessServiceGroupMember, 60, Info, "this={0} partitionId={1} instanceId={2} - Initialization succeeded", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_2_StatelessServiceGroupMember, 61, Info, "this={0} partitionId={1} instanceId={2} - Uninitialization", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_3_StatelessServiceGroupMember, 62, Info, "this={0} partitionId={1} instanceId={2} - Replica is brokered, resolving to brokered member", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Info_4_StatelessServiceGroupMember, 63, Info, "this={0} partitionId={1} instanceId={2} - Retrieving brokered service failed with {3}", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportMoveCost, 64, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} -System stateless service partition unavailable", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportInstanceHealth, 65, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} -System stateless service partition unavailable for ReportInstanceHealth", "id", "object", "instanceId"),
            SERVICEGROUPSTATELESS_STRUCTURED_TRACE(Error_0_StatelessReportPartitionHealth, 66, Error, Common::TraceChannelType::Debug, "this={1} instanceId={2} -System stateless service partition unavailable for ReportPartitionHealth", "id", "object", "instanceId")
        {
        }

        static ServiceGroupStatelessEventSource const & GetEvents();

    };
}
