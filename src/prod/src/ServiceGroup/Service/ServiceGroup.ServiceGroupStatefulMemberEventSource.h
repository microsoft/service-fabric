// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupStatefulMember, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupStatefulMemberEventSource
    {
    public:
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_0_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_0_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_1_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_1_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_2_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_2_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_3_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_3_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_4_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_5_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_4_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_6_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_7_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_8_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_5_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_9_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_10_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_6_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_11_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_12_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_7_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_13_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_14_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_8_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_9_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_15_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_16_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_17_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_18_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_10_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_11_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_12_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_13_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_14_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_19_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_20_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_15_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_16_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_21_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_22_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_23_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, BOOLEAN);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_24_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_25_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_26_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_27_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_17_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_18_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_19_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_20_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_21_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_28_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_29_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_30_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_2_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_31_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_23_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_24_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_25_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_32_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_26_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_33_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_34_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_35_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_27_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_36_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_28_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_37_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_29_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_38_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_30_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_39_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_31_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_40_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_32_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_41_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_33_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_34_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_42_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int64, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_35_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_43_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_36_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_44_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int64, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_37_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_45_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_38_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_46_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_39_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_47_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_40_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_48_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_49_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_3_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_50_StatefulServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);

        ServiceGroupStatefulMemberEventSource() :
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_0_StatefulServiceGroupMember, 4, Error, "this={1} partitionId={2} replicaId={3} - CreateReplica failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_0_StatefulServiceGroupMember, 5, Info, "this={1} partitionId={2} replicaId={3} - Initialization succeeded", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_1_StatefulServiceGroupMember, 6, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::BeginOpen call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_1_StatefulServiceGroupMember, 7, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::BeginOpen call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupMember, 8, Warning, "this={1} partitionId={2} replicaId={3} - Cleaning up failed service member open", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_2_StatefulServiceGroupMember, 9, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::EndOpen call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_2_StatefulServiceGroupMember, 10, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::EndOpen call failed - no call to CreateReplicator during open", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_3_StatefulServiceGroupMember, 11, Info, "this={1} partitionId={2} replicaId={3} - Success opening service member", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_3_StatefulServiceGroupMember, 12, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::EndOpen call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_4_StatefulServiceGroupMember, 13, Info, "this={1} partitionId={2} replicaId={3} - Service member already in role {4}, ignoring change role", "id", "object", "partitionId", "replicaId", "newRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_5_StatefulServiceGroupMember, 14, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::BeginChangeRole call from role {4} to role {5}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "newRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_4_StatefulServiceGroupMember, 15, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::BeginChangeRole from role {4} to role {5} failed with {6}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "newRole", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_6_StatefulServiceGroupMember, 16, Info, "this={1} partitionId={2} replicaId={3} - Service member already in role {4}, completing", "id", "object", "partitionId", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_7_StatefulServiceGroupMember, 17, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::EndChangeRole call from role {4} to role {5}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "proposedReplicaRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_8_StatefulServiceGroupMember, 18, Info, "this={1} partitionId={2} replicaId={3} - Success changing service member from role {4} to role {5}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "proposedReplicaRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_5_StatefulServiceGroupMember, 19, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::EndChangeRole from role {4} to role {5} failed with {6}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "proposedReplicaRole", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_9_StatefulServiceGroupMember, 20, Info, "this={1} partitionId={2} replicaId={3} - Service member is not opened, ignoring close", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_10_StatefulServiceGroupMember, 21, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::BeginClose call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_6_StatefulServiceGroupMember, 22, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::BeginClose call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_11_StatefulServiceGroupMember, 23, Info, "this={1} partitionId={2} replicaId={3} - Service member is not opened, completing", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_12_StatefulServiceGroupMember, 24, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::EndClose call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_7_StatefulServiceGroupMember, 25, Error, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::EndClose call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_13_StatefulServiceGroupMember, 26, Info, "this={1} partitionId={2} replicaId={3} - Service member is not opened, ignoring abort", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_14_StatefulServiceGroupMember, 27, Info, "this={1} partitionId={2} replicaId={3} - IFabricStatefulServiceReplica::Abort call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_8_StatefulServiceGroupMember, 28, Error, "this={1} partitionId={2} replicaId={3} - Simulated stateful service partition unavailable", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_9_StatefulServiceGroupMember, 29, Error, "this={1} partitionId={2} replicaId={3} - CreateReplicator may be called only once", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_15_StatefulServiceGroupMember, 30, Info, "this={1} partitionId={2} replicaId={3} - Calling simulated stateful service partition CreateReplicator", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_16_StatefulServiceGroupMember, 31, Info, "this={1} partitionId={2} replicaId={3} - Returning simulated state replicator and system fabric replicator", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_17_StatefulServiceGroupMember, 32, Info, "this={1} partitionId={2} replicaId={3} - Inner state provider does not support atomic groups", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_18_StatefulServiceGroupMember, 33, Info, "this={1} partitionId={2} replicaId={3} - Inner state provider supports atomic groups", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_10_StatefulServiceGroupMember, 34, Error, "this={1} partitionId={2} replicaId={3} - Could not allocate extented buffer operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_11_StatefulServiceGroupMember, 35, Error, "this={1} partitionId={2} replicaId={3} - Could not initialize extented buffer operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_12_StatefulServiceGroupMember, 36, Error, "this={1} partitionId={2} replicaId={3} - Serializing operation data failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_13_StatefulServiceGroupMember, 37, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Simulated stateful replicator unavailable", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_14_StatefulServiceGroupMember, 38, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Copy stream is closed", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_19_StatefulServiceGroupMember, 39, Info, "this={1} partitionId={2} replicaId={3} - Calling simulated stateful replicator GetCopyStream", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_20_StatefulServiceGroupMember, 40, Info, "this={1} partitionId={2} replicaId={3} - Copy stream is starting", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_15_StatefulServiceGroupMember, 41, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Simulated stateful replicator GetCopyStream call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_16_StatefulServiceGroupMember, 42, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Replication stream is closed", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_21_StatefulServiceGroupMember, 43, Info, "this={1} partitionId={2} replicaId={3} - Replication stream is starting", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_22_StatefulServiceGroupMember, 44, Info, "this={1} partitionId={2} replicaId={3} - Enqueue copy operation {4} with sequence number {5}", "id", "object", "partitionId", "replicaId", "operation", "sequenceNumber"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_23_StatefulServiceGroupMember, 45, Info, "this={1} partitionId={2} replicaId={3} - Enqueue replication operation {4} with sequence number {5}", "id", "object", "partitionId", "replicaId", "operation", "sequenceNumber"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupMember, 46, Warning, "this={1} partitionId={2} replicaId={3} - Forcing a drain of replication stream with null dispatch flag {4}", "id", "object", "partitionId", "replicaId", "waitForNullDispatch"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_24_StatefulServiceGroupMember, 47, Info, "this={1} partitionId={2} replicaId={3} - Clearing operation streams", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_25_StatefulServiceGroupMember, 48, Info, "this={1} partitionId={2} replicaId={3} - Replica is brokered, resolving to brokered member", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_26_StatefulServiceGroupMember, 49, Info, "this={1} partitionId={2} replicaId={3} - Retrieving brokered service failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_27_StatefulServiceGroupMember, 50, Info, "this={1} partitionId={2} replicaId={3} - Starting operation streams", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_17_StatefulServiceGroupMember, 51, Error, "this={1} partitionId={2} replicaId={3} - Replica is faulted", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_18_StatefulServiceGroupMember, 52, Error, "this={1} partitionId={2} replicaId={3} - Could not start copy operation stream", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_19_StatefulServiceGroupMember, 53, Error, "this={1} partitionId={2} replicaId={3} - Could not initialize copy operation stream", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_20_StatefulServiceGroupMember, 54, Error, "this={1} partitionId={2} replicaId={3} - Could not start replication operation stream", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_21_StatefulServiceGroupMember, 55, Error, "this={1} partitionId={2} replicaId={3} - Could not initialize replication operation stream", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_28_StatefulServiceGroupMember, 56, Info, "this={1} partitionId={2} replicaId={3} - Preemptively enqueuing NULL copy operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_29_StatefulServiceGroupMember, 57, Info, "this={1} partitionId={2} replicaId={3} - Drain for update epoch in current replica role {4}", "id", "object", "partitionId", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_30_StatefulServiceGroupMember, 58, Info, "this={1} partitionId={2} replicaId={3} - Waiting for copy operation stream to drain", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_2_StatefulServiceGroupMember, 59, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Copy operation stream drain failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_31_StatefulServiceGroupMember, 60, Info, "this={1} partitionId={2} replicaId={3} - Copy operation stream drained", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_23_StatefulServiceGroupMember, 61, Error, "this={1} partitionId={2} replicaId={3} - Could not allocate update epoch operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_24_StatefulServiceGroupMember, 62, Error, "this={1} partitionId={2} replicaId={3} - Could not initialize update epoch operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_25_StatefulServiceGroupMember, 63, Error, "this={1} partitionId={2} replicaId={3} - Could not enqueue update epoch operation", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_32_StatefulServiceGroupMember, 64, Info, "this={1} partitionId={2} replicaId={3} - Waiting for replication operation stream to drain", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_26_StatefulServiceGroupMember, 65, Error, "this={1} partitionId={2} replicaId={3} - Update epoch barrier failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_33_StatefulServiceGroupMember, 66, Info, "this={1} partitionId={2} replicaId={3} - Replication operation stream drained", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_34_StatefulServiceGroupMember, 67, Info, "this={1} partitionId={2} replicaId={3} - Drain for update epoch succeeded", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_35_StatefulServiceGroupMember, 68, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStateProvider::BeginUpdateEpoch call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_27_StatefulServiceGroupMember, 69, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::BeginUpdateEpoch call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_36_StatefulServiceGroupMember, 70, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::EndUpdateEpoch call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_28_StatefulServiceGroupMember, 71, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::EndUpdateEpoch call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_37_StatefulServiceGroupMember, 72, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStateProvider::GetLastCommittedSequenceNumber call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_29_StatefulServiceGroupMember, 73, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::GetLastCommittedSequenceNumber call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_38_StatefulServiceGroupMember, 74, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStateProvider::BeginOnDataLoss call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_30_StatefulServiceGroupMember, 75, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::BeginOnDataLoss call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_39_StatefulServiceGroupMember, 76, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStatefulServiceReplica::EndOnDataLoss call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_31_StatefulServiceGroupMember, 77, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::EndOnDataLoss call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_40_StatefulServiceGroupMember, 78, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStateProvider::GetCopyContext call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_32_StatefulServiceGroupMember, 79, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::GetCopyContext call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_41_StatefulServiceGroupMember, 80, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricStateProvider::GetCopyState call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_33_StatefulServiceGroupMember, 81, Error, "this={1} partitionId={2} replicaId={3} - IFabricStateProvider::GetCopyState call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_34_StatefulServiceGroupMember, 82, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Simulated atomic group stateful replicator unavailable", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_42_StatefulServiceGroupMember, 83, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::BeginAtomicGroupCommit(AtomicGroupId={4}, CommitSequenceNumber={5}) call", "id", "object", "partitionId", "replicaId", "atomicGroupId", "commitSequenceNumber"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_35_StatefulServiceGroupMember, 84, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::BeginAtomicGroupCommit(AtomicGroupId={4}) call failed with {5}", "id", "object", "partitionId", "replicaId", "atomicGroupId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_43_StatefulServiceGroupMember, 85, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::EndAtomicGroupCommit call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_36_StatefulServiceGroupMember, 86, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::EndAtomicGroupCommit call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_44_StatefulServiceGroupMember, 87, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::BeginAtomicGroupRollback(AtomicGroupId={4}, RollbackSequenceNumber={5}) call", "id", "object", "partitionId", "replicaId", "atomicGroupId", "rollbackSequenceNumber"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_37_StatefulServiceGroupMember, 88, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::BeginAtomicGroupRollback(AtomicGroupId={4}) call failed with {5}", "id", "object", "partitionId", "replicaId", "atomicGroupId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_45_StatefulServiceGroupMember, 89, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::EndAtomicGroupRollback call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_38_StatefulServiceGroupMember, 90, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::EndAtomicGroupRollback call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_46_StatefulServiceGroupMember, 91, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::BeginUndoProgress(FromCommitSequenceNumber={4}) call", "id", "object", "partitionId", "replicaId", "fromCommitSequenceNumber"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_39_StatefulServiceGroupMember, 92, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::BeginUndoProgress call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_47_StatefulServiceGroupMember, 93, Info, "this={1} partitionId={2} replicaId={3} - Starting IFabricAtomicGroupStateProvider::EndUndoProgress call", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Error_40_StatefulServiceGroupMember, 94, Error, "this={1} partitionId={2} replicaId={3} - IFabricAtomicGroupStateProvider::EndUndoProgress call failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_48_StatefulServiceGroupMember, 95, Info, "this={1} partitionId={2} replicaId={3} - Service member already updated its operation streams", "id", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_49_StatefulServiceGroupMember, 96, Info, "this={1} partitionId={2} replicaId={3} - Service member updating operation streams - current role is {4} new role is {5}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Warning_3_StatefulServiceGroupMember, 97, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Replication operation stream drain failed with {4}", "id", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFULMEMBER_STRUCTURED_TRACE(Info_50_StatefulServiceGroupMember, 98, Info, "this={1} partitionId={2} replicaId={3} - Service member closing", "id", "object", "partitionId", "replicaId")
        {
        }

        static ServiceGroupStatefulMemberEventSource const & GetEvents();

    };
}
