// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPREPLICATION_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupReplication, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupReplicationEventSource
    {
    public:
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_EmptyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_EmptyOperationDataAsyncEnumerator, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_EmptyOperationDataAsyncEnumerator, uintptr_t, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_EmptyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CopyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_CopyOperationDataAsyncEnumerator, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CopyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_CopyOperationDataAsyncEnumerator, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CopyOperationDataAsyncEnumerator, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_CopyOperationDataAsyncEnumerator, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_CopyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_CopyOperationDataAsyncEnumerator, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_CopyOperationDataAsyncEnumerator, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CompositeAtomicGroupCommitRollbackOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CompositeAtomicGroupCommitRollbackOperationAsync, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CompositeAtomicGroupCommitRollbackOperationAsync, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_ReplicationAtomicGroupOperationCallback, uintptr_t, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_AtomicGroupAsyncOperationContext, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_AtomicGroupCommitRollbackOperation, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, uint32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_StatefulServiceMemberCopyContextProcessing, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, int32, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyContext, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, int64, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_7_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64, std::wstring, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_8_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupCopyState, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, std::wstring, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_13_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_14_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int32, int64, std::wstring, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_13_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_7_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_14_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_8_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_15_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_9_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_10_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int32, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_16_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_11_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_17_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_12_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_18_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int32, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_13_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_14_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_15_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int32, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_19_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int32, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_20_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int32, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_21_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_22_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_23_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_24_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_25_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_26_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_27_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_28_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_16_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_17_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_29_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_30_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_31_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_32_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_33_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_34_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_35_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_36_StatefulServiceGroupAtomicGroupReplicationProcessing, std::wstring, uintptr_t, int64, int64, std::wstring);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_CopyContextStreamDispatcher, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CopyContextStreamDispatcher, uintptr_t, int32, size_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CopyContextStreamDispatcher, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_CopyContextStreamDispatcher, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_CopyContextStreamDispatcher, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CopyContextStreamDispatcher, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_CopyContextStreamDispatcher, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_CopyContextStreamDispatcher, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_CopyContextStreamDispatcher, uintptr_t, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupOperationStreams, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_OperationDataFactory, uintptr_t);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupReplicationProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_3_StatefulServiceGroupCopyProcessing, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_OperationStream_StatefulServiceMember, std::wstring, uintptr_t, Common::Guid, int64);
        
        ServiceGroupReplicationEventSource() :
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_EmptyOperationDataAsyncEnumerator, 4, Error, "this={0} - Could not allocate operation context", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_EmptyOperationDataAsyncEnumerator, 5, Error, "this={0} - Operation context initialization failed with {1}", "object", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_EmptyOperationDataAsyncEnumerator, 6, Info, "this={0} - Empty operation context {1} created", "object", "context"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_EmptyOperationDataAsyncEnumerator, 7, Info, "this={0} - Returning NULL operation data", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CopyOperationDataAsyncEnumerator, 8, Info, "this={0} - BeginGetNext all enumerators have already been processed", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_CopyOperationDataAsyncEnumerator, 9, Error, Common::TraceChannelType::Debug, "this={0} - {1} Service BeginGetNext failed with {2}", "object", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CopyOperationDataAsyncEnumerator, 10, Info, "this={0} - EndGetNext all enumerators have already been processed", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_CopyOperationDataAsyncEnumerator, 11, Error, Common::TraceChannelType::Debug, "this={0} - {1} Service EndGetNext failed with {2}", "object", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CopyOperationDataAsyncEnumerator, 12, Info, "this={0} - {1} Service EndGetNext returned non-NULL operation data", "object", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_CopyOperationDataAsyncEnumerator, 13, Info, "this={0} - {1} Including NULL operation", "object", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_CopyOperationDataAsyncEnumerator, 14, Error, "this={0} - Failed to allocate extended outgoing buffer", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_CopyOperationDataAsyncEnumerator, 15, Error, "this={0} - Failed to initialize extended outgoing buffer", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_CopyOperationDataAsyncEnumerator, 16, Error, "this={0} - Serializing operation data failed with {1}", "object", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CompositeAtomicGroupCommitRollbackOperationAsync, 17, Info, "this={0} partitionId={1} - Beginning operation", "object", "partitionId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CompositeAtomicGroupCommitRollbackOperationAsync, 18, Info, "this={0} partitionId={1} - AtomicGroupId={2} committed", "object", "partitionId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CompositeAtomicGroupCommitRollbackOperationAsync, 19, Info, "this={0} partitionId={1} - AtomicGroupId={2} rolled back", "object", "partitionId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_ReplicationAtomicGroupOperationCallback, 20, Info, "this={0} - System context {1} completed synchronously", "object", "context"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_AtomicGroupAsyncOperationContext, 21, Info, "this={0} - Cancel not supported", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_AtomicGroupCommitRollbackOperation, 22, Info, "this={0} - Atomic group {1} was {2}", "object", "atomicGroupId", "committedrolledback"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceMemberCopyContextProcessing, 23, Error, "this={0} partitionId={1} - Could not allocate operation context", "object", "partitionId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceMemberCopyContextProcessing, 24, Error, "this={0} partitionId={1} - Initialization of operation context {2} failed with {3}", "object", "partitionId", "context", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceMemberCopyContextProcessing, 25, Warning, Common::TraceChannelType::Debug, "this={0} partitionId={1} - ErrorCode {2} seen in the operation data stream", "object", "partitionId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceMemberCopyContextProcessing, 26, Warning, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Operation stream already ended", "object", "partitionId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceMemberCopyContextProcessing, 27, Info, "this={0} partitionId={1} - Signal that the last operation (NULL) has been dispatched", "object", "partitionId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceMemberCopyContextProcessing, 28, Error, "this={0} partitionId={1} - Could not store operation context {2}", "object", "partitionId", "context"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceMemberCopyContextProcessing, 29, Error, "this={0} partitionId={1} - CreateEvent failed with {2}", "object", "partitionId", "win32LastError"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceMemberCopyContextProcessing, 30, Error, "this={0} partitionId={1} - Could not enqueue operation data {2}", "object", "partitionId", "operationData"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceMemberCopyContextProcessing, 31, Info, "this={0} partitionId={1} - Signal that the error code {2} has been dispatched", "object", "partitionId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceMemberCopyContextProcessing, 32, Info, "this={0} partitionId={1} - Signal that the last operation data {2} has been dispatched", "object", "partitionId", "text"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_StatefulServiceMemberCopyContextProcessing, 33, Warning, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Abandoning operation {2}", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_OperationStream_StatefulServiceMember, 34, Error, "this={1} partitionId={2} - Could not allocate operation context ({3})", "id", "object", "partitionId", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_OperationStream_StatefulServiceMember, 35, Error, "this={1} partitionId={2} - Initialization of operation context {3} failed with {4} for {5}", "id", "object", "partitionId", "context", "errorCode", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_OperationStream_StatefulServiceMember, 36, Info, "this={1} partitionId={2} - Operation stream already ended ({3})", "id", "object", "partitionId", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_OperationStream_StatefulServiceMember, 37, Info, "this={1} partitionId={2} - Completing update epoch operation {3} in {4}", "id", "object", "partitionId", "operationUpdateEpoch", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_OperationStream_StatefulServiceMember, 38, Info, "this={1} partitionId={2} - Signal that the last operation (NULL) has been dispatched ({3})", "id", "object", "partitionId", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_OperationStream_StatefulServiceMember, 39, Error, "this={1} partitionId={2} - Could not store operation context {3} in {4}", "id", "object", "partitionId", "context", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_OperationStream_StatefulServiceMember, 40, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Dropping operation {3} since the {4} was drained/faulted", "id", "object", "partitionId", "operation", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_OperationStream_StatefulServiceMember, 41, Error, "this={1} partitionId={2} - Could not enqueue operation {3} in {4}", "id", "object", "partitionId", "operation", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_OperationStream_StatefulServiceMember, 42, Info, "this={1} partitionId={2} - Completing update epoch operation {3} in {4}", "id", "object", "partitionId", "operationUpdateEpoch", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_OperationStream_StatefulServiceMember, 43, Info, "this={1} partitionId={2} - Signal that the last operation (NULL) has been dispatched in {3}", "id", "object", "partitionId", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_OperationStream_StatefulServiceMember, 44, Info, "this={1} partitionId={2} - {3} already drained with {4}", "id", "object", "partitionId", "streamName", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_OperationStream_StatefulServiceMember, 45, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Abandoning operation {3} from {4}", "id", "object", "partitionId", "operation", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_OperationStream_StatefulServiceMember, 46, Info, "this={1} partitionId={2} - Waiting for {3} to drain {4} operations", "id", "object", "partitionId", "streamName", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_OperationStream_StatefulServiceMember, 47, Info, "this={1} partitionId={2} - Drain completed for {3}", "id", "object", "partitionId", "streamName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_OperationStream_StatefulServiceMember, 48, Info, "this={1} partitionId={2} - Enqueue update epoch operation", "id", "object", "partitionId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_OperationStream_StatefulServiceMember, 49, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Completing update epoch operation {3} (stream faulted)", "id", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_OperationStream_StatefulServiceMember, 50, Info, "this={1} partitionId={2} - Completing update epoch operation {3} (stream closed)", "id", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_OperationStream_StatefulServiceMember, 51, Info, "this={1} partitionId={2} - Storing update epoch operation {3} (waiting for waiter)", "id", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_OperationStream_StatefulServiceMember, 52, Info, "this={1} partitionId={2} - Completing update epoch operation {3} (waiter present)", "id", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_OperationStream_StatefulServiceMember, 53, Info, "this={1} partitionId={2} - Storing update epoch operation {3} (non-empty stream)", "id", "object", "partitionId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyContext, 54, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - {3} - GetCopyContext failed with {4}", "id", "object", "replicaId", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyContext, 55, Info, "this={1} replicaId={2} - {3} - Service returns NULL copy context enumerator.  Insert an empty enumerater", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyContext, 56, Error, "this={1} replicaId={2} - {3} - Failed to allocate empty operation data async enumerator", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyContext, 57, Error, "this={1} replicaId={2} - {3} - Failed to allocate operation data async enumerator wrapper", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyContext, 58, Error, "this={1} replicaId={2} - {3} Failed to insert to copyContextEnumerators with {4}", "id", "object", "replicaId", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyContext, 59, Info, "this={1} replicaId={2} - all NULL enumerators", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyContext, 60, Info, "this={1} replicaId={2} - Returning {3} copy context enumerators", "id", "object", "replicaId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyContext, 61, Info, "this={1} replicaId={2} - Starting copy context", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyContext, 62, Error, "this={1} replicaId={2} - Failed to allocate operation data async enumerator", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyState, 63, Error, "this={1} replicaId={2} - Could not allocate atomic group state enumerator", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyState, 64, Info, "this={1} replicaId={2} - Updating the copy state sequence number from {3} to last committed sequence number {4}", "id", "object", "replicaId", "uptoSequenceNumber", "lastCommittedLSN"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyState, 65, Info, "this={1} replicaId={2} - Updating the copy state sequence number from {3} to previous epoch sequence number {4}", "id", "object", "replicaId", "uptoSequenceNumber", "previousEpochLsn"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyState, 66, Info, "this={1} replicaId={2} - Using copy state sequence number {3}", "id", "object", "replicaId", "uptoSequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyState, 67, Info, "this={1} replicaId={2} - No atomic groups to copy", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupCopyState, 68, Info, "this={1} replicaId={2} - Skipping atomic group {3} with created LSN {4}", "id", "object", "replicaId", "member", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupCopyState, 69, Info, "this={1} replicaId={2} - Copying atomic group {3} with created LSN {4}", "id", "object", "replicaId", "member", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupCopyState, 70, Info, "this={1} replicaId={2} - Skipping participant {3} with created LSN {4} from atomic group {5}", "id", "object", "replicaId", "member", "sequenceNumber", "atomicGroup"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupCopyState, 71, Info, "this={1} replicaId={2} - Copying participant {3} with created LSN {4} from atomic group {5}", "id", "object", "replicaId", "member", "sequenceNumber", "atomicGroup"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyState, 72, Error, "this={1} replicaId={2} - Could not initialize atomic group state enumerator", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyState, 73, Error, "this={1} replicaId={2} - GetCopyState for {3} failed with {4}", "id", "object", "replicaId", "serviceName", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupCopyState, 74, Info, "this={1} replicaId={2} - {3} - Service returns NULL copy state enumerator. Inserting an empty enumerator", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyState, 75, Error, "this={1} replicaId={2} - {3} - Failed to insert to serviceCopyStateEnumerators with {4}", "id", "object", "replicaId", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupCopyState, 76, Info, "this={1} replicaId={2} - Returning 1 atomic group state enumerator and {3} copy state enumerators", "id", "object", "replicaId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupCopyState, 77, Info, "this={1} replicaId={2} - Start copy state in epoch ({3},{4}) and role {5}", "id", "object", "replicaId", "dataLossNumber", "configurationNumber", "currentReplicaRole"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyState, 78, Error, "this={1} replicaId={2} - Failed to allocate copy operation data async enumerator", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupCopyState, 79, Error, "this={1} replicaId={2} - Failed to allocate copy context operation data stream for service member {3}", "id", "object", "replicaId", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupCopyState, 80, Error, "this={1} replicaId={2} - Failed to initialize copy context operation data stream for service member {3}", "id", "object", "replicaId", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_7_StatefulServiceGroupCopyState, 81, Error, "this={1} replicaId={2} - Inserting copy context operation data stream for service member {3} failed with {4}", "id", "object", "replicaId", "serviceName", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_8_StatefulServiceGroupCopyState, 82, Error, "this={1} replicaId={2} - Failed to allocate copy context stream dispatcher", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupCopyState, 83, Info, "this={1} replicaId={2} - Start to dispatch system copy context stream to services", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupReplicationProcessing, 84, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful replicator unavailable", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupCopyProcessing, 85, Info, "this={1} replicaId={2} - Copy stream already started", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupCopyProcessing, 86, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Copy stream is closed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupCopyProcessing, 87, Info, "this={1} replicaId={2} - Starting draining copy stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupReplicationProcessing, 88, Info, "this={1} replicaId={2} - Replication stream already started", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupReplicationProcessing, 89, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Replication stream is closed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupReplicationProcessing, 90, Info, "this={1} replicaId={2} - Starting draining replication stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupCopyProcessing, 91, Error, "this={1} replicaId={2} - Copy IFabricOperationStream::BeginGetOperation failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupReplicationProcessing, 92, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Replication IFabricOperationStream::BeginGetOperation failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupCopyProcessing, 93, Info, "this={1} replicaId={2} - Copy operation stream processing completed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupReplicationProcessing, 94, Info, "this={1} replicaId={2} - Replication operation stream processing completed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOperationStreams, 95, Info, "this={1} replicaId={2} - Starting inner operation streams", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOperationStreams, 96, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOperationStreams, 97, Info, "this={1} replicaId={2} - Starting operation streams", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupOperationStreams, 98, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Operation streams have started already", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOperationStreams, 99, Error, "this={1} replicaId={2} - Could not retrieve system replication operation stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOperationStreams, 100, Error, "this={1} replicaId={2} - Could not retrieve system copy operation stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupOperationStreams, 101, Error, "this={1} replicaId={2} - Could not create replication operation stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupOperationStreams, 102, Error, "this={1} replicaId={2} - Could not create copy operation stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupOperationStreams, 103, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Start inner operation streams failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOperationStreams, 104, Info, "this={1} replicaId={2} - Preemptively draining copy operation stream", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupCopyProcessing, 105, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Copy IFabricOperationStream::EndGetOperation failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupCopyProcessing, 106, Info, "this={1} replicaId={2} - Processing last copy operation (NULL)", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupCopyProcessing, 107, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Atomic groups state was not seen", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupCopyProcessing, 108, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Not all NULL operations have been seen", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupCopyProcessing, 109, Info, "this={1} replicaId={2} - Signal last copy operation (NULL) has been dispatched", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupCopyProcessing, 110, Info, "this={1} replicaId={2} - Processing copy operation w/ sequence number {3}", "id", "object", "replicaId", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupCopyProcessing, 111, Info, "this={1} replicaId={2} - Service group processing NULL copy state operation for service member {3}", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupCopyProcessing, 112, Info, "this={1} replicaId={2} - Service group processing NULL copy state operation for service group", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupCopyProcessing, 113, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not deserialize atomic group state", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupCopyProcessing, 114, Info, "this={1} replicaId={2} - Epoch set to ({3},{4})", "id", "object", "replicaId", "dataLossNumber", "configurationNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupCopyProcessing, 115, Info, "this={1} replicaId={2} - Received atomic group {3} with created LSN {4}", "id", "object", "replicaId", "member", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupCopyProcessing, 116, Info, "this={1} replicaId={2} - Received participant {3} with created LSN {4} from atomic group {5}", "id", "object", "replicaId", "member", "sequenceNumber", "atomicGroup"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupCopyProcessing, 117, Info, "this={1} replicaId={2} - Atomic group sequence id set to {3}", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_StatefulServiceGroupCopyProcessing, 118, Info, "this={1} replicaId={2} - Last committed LSN is set to {3}", "id", "object", "replicaId", "lastCommittedLSN"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_13_StatefulServiceGroupCopyProcessing, 119, Info, "this={1} replicaId={2} - Atomic group state retrieved", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupCopyProcessing, 120, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not dispatch copy operation {3} to service {4}", "id", "object", "replicaId", "operation", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_14_StatefulServiceGroupCopyProcessing, 121, Info, "this={1} replicaId={2} - Dispatching copy operation {3} to service {4}", "id", "object", "replicaId", "buffer", "serviceGroupMemberPartition"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_StatefulServiceGroupAtomicGroupReplicationProcessing, 122, Info, "this={1} replicaId={2} - Processing atomic group {3}", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_StatefulServiceGroupAtomicGroupReplicationProcessing, 123, Info, "this={1} replicaId={2} - Dispatching atomic group {3} create replication operation {4} to service {5}", "id", "object", "replicaId", "atomicGroupId", "atomicGroupOperation", "serviceGroupMemberPartition"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_StatefulServiceGroupAtomicGroupReplicationProcessing, 124, Info, "this={1} replicaId={2} - Dispatching atomic group {3} replication operation {4} to service {5}", "id", "object", "replicaId", "atomicGroupId", "atomicGroupOperation", "serviceGroupMemberPartition"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupAtomicGroupReplicationProcessing, 125, Info, "this={1} replicaId={2} - Dispatching atomic group {3} commit replication operation {4} to service {5}", "id", "object", "replicaId", "atomicGroupId", "atomicGroupOperation", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupAtomicGroupReplicationProcessing, 126, Info, "this={1} replicaId={2} - Dispatching atomic group {3} rollback replication operation {4} to service {5}", "id", "object", "replicaId", "atomicGroupId", "atomicGroupOperation", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupReplicationProcessing, 127, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Replication IFabricOperationStream::EndGetOperation failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupReplicationProcessing, 128, Info, "this={1} replicaId={2} - Processing last replication operation (NULL)", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupReplicationProcessing, 129, Info, "this={1} replicaId={2} - Signal last replication operation (NULL) has been dispatched", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupReplicationProcessing, 130, Info, "this={1} replicaId={2} - Processing replication operation {3}", "id", "object", "replicaId", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupReplicationProcessing, 131, Info, "this={1} replicaId={2} - Dropping replication operation {3}", "id", "object", "replicaId", "sequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupReplicationProcessing, 132, Info, "this={1} replicaId={2} - Adjusting atomic group state by removing atomic group {3}, Remaining atomic groups in-flight {4}", "id", "object", "replicaId", "atomicGroupId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupReplicationProcessing, 133, Info, "this={1} replicaId={2} - Service group processing replication operation {3}", "id", "object", "replicaId", "operation"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupReplicationProcessing, 134, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not dispatch atomic group {3} replication operation {4} to service {5}", "id", "object", "replicaId", "atomicGroupId", "operation", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupReplicationProcessing, 135, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not dispatch replication operation {3} to service {4}", "id", "object", "replicaId", "operation", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupReplicationProcessing, 136, Info, "this={1} replicaId={2} - Dispatching replication operation {3} to service {4}", "id", "object", "replicaId", "buffer", "serviceGroupMemberPartition"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupAtomicGroupReplicationProcessing, 137, Info, "this={1} replicaId={2} - Starting operation type {3} for atomic group {4} for participant {5}", "id", "object", "replicaId", "operationType", "atomicGroupId", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupAtomicGroupReplicationProcessing, 138, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Write status not granted", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_StatefulServiceGroupAtomicGroupReplicationProcessing, 139, Error, "this={1} replicaId={2} - Refresh atomic group failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_1_StatefulServiceGroupAtomicGroupReplicationProcessing, 140, Error, "this={1} replicaId={2} - Serializing operation data failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupAtomicGroupReplicationProcessing, 141, Info, "this={1} replicaId={2} - BeginReplicate operation type {3} for atomic group {4} for participant {5} succeeded with sequenceNumber={6}", "id", "object", "replicaId", "operationType", "atomicGroupId", "serviceName", "operationSequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_2_StatefulServiceGroupAtomicGroupReplicationProcessing, 142, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - BeginReplicate operation type {3} for atomic group {4} for participant {5} failed with {6}", "id", "object", "replicaId", "operationType", "atomicGroupId", "serviceName", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupAtomicGroupReplicationProcessing, 143, Info, "this={1} replicaId={2} - EndReplicate operation for atomic group {3} for participant {4} succeeded for sequenceNumber={5}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_StatefulServiceGroupAtomicGroupReplicationProcessing, 144, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - EndReplicate operation for atomic group {3} for participant {4} with sequenceNumber={5} failed with {6}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupAtomicGroupReplicationProcessing, 145, Info, "this={1} replicaId={2} - EndReplicate commit for atomic group {3} for participant {4} succeeded for sequenceNumber={5}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_StatefulServiceGroupAtomicGroupReplicationProcessing, 146, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - EndReplicate commit for atomic group {3} for participant {4} with sequenceNumber={5} failed with {6}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupAtomicGroupReplicationProcessing, 147, Info, "this={1} replicaId={2} - EndReplicate rollback for atomic group {3} for participant {4} succeeded for sequenceNumber={5}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_StatefulServiceGroupAtomicGroupReplicationProcessing, 148, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - EndReplicate rollback for atomic group {3} for participant {4} with sequenceNumber={5} failed with {6}", "id", "object", "replicaId", "atomicGroupId", "serviceName", "operationSequenceNumber", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupAtomicGroupReplicationProcessing, 149, Info, "this={1} replicaId={2} - Starting commit on (AtomicGroupId={3}, CommitSequenceNumber={4}, Status={5})", "id", "object", "replicaId", "atomicGroupId", "sequenceNumber", "status"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_11_StatefulServiceGroupAtomicGroupReplicationProcessing, 150, Info, "this={1} replicaId={2} - Starting rollback on (AtomicGroupId={3}, RollbackSequenceNumber={4}, Status={5})", "id", "object", "replicaId", "atomicGroupId", "sequenceNumber", "status"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_12_StatefulServiceGroupAtomicGroupReplicationProcessing, 151, Info, "this={1} replicaId={2} - AtomicGroupId={3} commit/rollback processing in progress", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupAtomicGroupReplicationProcessing, 152, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Participant {3} attempted to commit/rollback unknown atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_13_StatefulServiceGroupAtomicGroupReplicationProcessing, 153, Info, "this={1} replicaId={2} - Adding new atomic group {3}, New atomic groups in-flight {4}", "id", "object", "replicaId", "atomicGroupId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_7_StatefulServiceGroupAtomicGroupReplicationProcessing, 154, Error, "this={1} replicaId={2} - Could not store new atomic group {3}, Remaining atomic groups in-flight {4}", "id", "object", "replicaId", "atomicGroupId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_14_StatefulServiceGroupAtomicGroupReplicationProcessing, 155, Info, "this={1} replicaId={2} - Adding first participant {3} to atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_8_StatefulServiceGroupAtomicGroupReplicationProcessing, 156, Error, "this={1} replicaId={2} - Could not store first participant {3} in atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_15_StatefulServiceGroupAtomicGroupReplicationProcessing, 157, Info, "this={1} replicaId={2} - Found existent atomic group {3}", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_9_StatefulServiceGroupAtomicGroupReplicationProcessing, 158, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Unknown participant {3} attempted to commit/rollback atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_10_StatefulServiceGroupAtomicGroupReplicationProcessing, 159, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Unknown participant {3} initiated operation type {4} for atomic group {5} while the atomic group is in commit/rollback", "id", "object", "replicaId", "serviceName", "operationType", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_16_StatefulServiceGroupAtomicGroupReplicationProcessing, 160, Info, "this={1} replicaId={2} - Adding new participant {3} to existent atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_11_StatefulServiceGroupAtomicGroupReplicationProcessing, 161, Error, "this={1} replicaId={2} - Could not store new participant {3} in atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_17_StatefulServiceGroupAtomicGroupReplicationProcessing, 162, Info, "this={1} replicaId={2} - Found existent participant {3} in atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_12_StatefulServiceGroupAtomicGroupReplicationProcessing, 163, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Participant {3} is retrying to terminate atomic group {4} with incompatible operation", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_18_StatefulServiceGroupAtomicGroupReplicationProcessing, 164, Info, "this={1} replicaId={2} - Participant {3} is retrying operation type {4} for atomic group {5}", "id", "object", "replicaId", "serviceName", "operationType", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_13_StatefulServiceGroupAtomicGroupReplicationProcessing, 165, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Participant {3} is attempting to commit/rollback atomic group {4} multiple times", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_14_StatefulServiceGroupAtomicGroupReplicationProcessing, 166, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Participant {3} is attempting to commit/rollback atomic group {4} which was already set to commit/rollback by participant {5}", "id", "object", "replicaId", "serviceName", "atomicGroupId", "terminator"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_15_StatefulServiceGroupAtomicGroupReplicationProcessing, 167, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Participant {3} initiated operation type {4} for atomic group {5} while the atomic group is in commit/rollback", "id", "object", "replicaId", "serviceName", "operationType", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_19_StatefulServiceGroupAtomicGroupReplicationProcessing, 168, Info, "this={1} replicaId={2} - Participant {3} initiated operation type {4} for atomic group {5} while the atomic group still has {6} in-flight operations", "id", "object", "replicaId", "serviceName", "operationType", "atomicGroupId", "replicating"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupAtomicGroupReplicationProcessing, 169, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Atomic group {3} is already set to commit/rollback", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_20_StatefulServiceGroupAtomicGroupReplicationProcessing, 170, Info, "this={1} replicaId={2} - Participant {3} successfully replicated {4} operations in atomic group {5}", "id", "object", "replicaId", "serviceName", "replicated", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_21_StatefulServiceGroupAtomicGroupReplicationProcessing, 171, Info, "this={1} replicaId={2} - Atomic group {3} successfully replicated {4} operations", "id", "object", "replicaId", "atomicGroupId", "replicated"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_22_StatefulServiceGroupAtomicGroupReplicationProcessing, 172, Info, "this={1} replicaId={2} - No atomics groups found", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_23_StatefulServiceGroupAtomicGroupReplicationProcessing, 173, Info, "this={1} replicaId={2} - At least atomic group {3} is not done replicating", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_24_StatefulServiceGroupAtomicGroupReplicationProcessing, 174, Info, "this={1} replicaId={2} - All atomics groups are done replicating", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_25_StatefulServiceGroupAtomicGroupReplicationProcessing, 175, Info, "this={1} replicaId={2} - Waiting for atomic group {3} to drain", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_26_StatefulServiceGroupAtomicGroupReplicationProcessing, 176, Info, "this={1} replicaId={2} - Waiting for all atomic groups to drain", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_27_StatefulServiceGroupAtomicGroupReplicationProcessing, 177, Info, "this={1} replicaId={2} - All atomic groups have drained", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_28_StatefulServiceGroupAtomicGroupReplicationProcessing, 178, Info, "this={1} replicaId={2} - Removing all {3} atomic groups", "id", "object", "replicaId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_16_StatefulServiceGroupAtomicGroupReplicationProcessing, 179, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Removing participant {3} from atomic group {4}", "id", "object", "replicaId", "serviceName", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_17_StatefulServiceGroupAtomicGroupReplicationProcessing, 180, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Removing atomic group {3}, Remaining atomic groups in-flight {4}", "id", "object", "replicaId", "atomicGroupId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_29_StatefulServiceGroupAtomicGroupReplicationProcessing, 181, Info, "this={1} replicaId={2} - Skipping rollback for atomic group {3}", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_30_StatefulServiceGroupAtomicGroupReplicationProcessing, 182, Info, "this={1} replicaId={2} - Starting rollback for atomic group {3}", "id", "object", "replicaId", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_31_StatefulServiceGroupAtomicGroupReplicationProcessing, 183, Info, "this={1} replicaId={2} - Atomic group {3} termination was retried and succeeded", "id", "object", "replicaId", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_32_StatefulServiceGroupAtomicGroupReplicationProcessing, 184, Info, "this={1} replicaId={2} - Removing atomic group {3}, Remaining atomic groups in-flight {4}", "id", "object", "replicaId", "atomicGroupId", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_33_StatefulServiceGroupAtomicGroupReplicationProcessing, 185, Info, "this={1} replicaId={2} - Updating last committed LSN from {3} to {4}", "id", "object", "replicaId", "lastCommittedLSN", "lsn"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_34_StatefulServiceGroupAtomicGroupReplicationProcessing, 186, Info, "this={1} replicaId={2} - Resetting last committed LSN from {3} to {4}", "id", "object", "replicaId", "lastCommittedLSN", "lsn"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_35_StatefulServiceGroupAtomicGroupReplicationProcessing, 187, Info, "this={1} replicaId={2} - Setting created LSN {3} for atomic group {4}", "id", "object", "replicaId", "sequenceNumber", "atomicGroupId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_36_StatefulServiceGroupAtomicGroupReplicationProcessing, 188, Info, "this={1} replicaId={2} - Setting created LSN {3} for participant {4}", "id", "object", "replicaId", "sequenceNumber", "serviceName"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_2_CopyContextStreamDispatcher, 189, Warning, Common::TraceChannelType::Debug, "this={0} - System copy context enumerator BeginGetNext failed with {1}", "object", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_0_CopyContextStreamDispatcher, 190, Info, "this={0} - FinishDispatch is called with {1} for {2} remaining service members", "object", "errorCode", "count"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_1_CopyContextStreamDispatcher, 191, Info, "this={0} - Finish copy context enumerator {1} with error code {2}", "object", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_CopyContextStreamDispatcher, 192, Warning, Common::TraceChannelType::Debug, "this={0} - Force drain copy context enumerator {1} with error code {2}", "object", "member", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_CopyContextStreamDispatcher, 193, Warning, Common::TraceChannelType::Debug, "this={0} - System copy context enumerator EndGetNext failed with {1}", "object", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_2_CopyContextStreamDispatcher, 194, Info, "this={0} - NULL operationData - Finish copy context enumerator of all services with success", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_3_CopyContextStreamDispatcher, 195, Error, "this={0} - Failed to allocate object for shrinked operation data", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_CopyContextStreamDispatcher, 196, Error, "this={0} - Failed to enqueue operation data to {1}", "object", "member"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupOperationStreams, 197, Warning, "this={1} replicaId={2} - Operations streams already completed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOperationStreams, 198, Info, "this={1} replicaId={2} - Service replica role is {3}", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOperationStreams, 199, Info, "this={1} replicaId={2} - Update operation streams failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_5_StatefulServiceGroupOperationStreams, 200, Info, "this={1} replicaId={2} - Service replica closing", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_6_StatefulServiceGroupOperationStreams, 201, Info, "this={1} replicaId={2} - Waiting for copy operation stream to complete", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_7_StatefulServiceGroupOperationStreams, 202, Info, "this={1} replicaId={2} - Copy operation stream completed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_8_StatefulServiceGroupOperationStreams, 203, Info, "this={1} replicaId={2} - Waiting for replication operation stream to complete", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_9_StatefulServiceGroupOperationStreams, 204, Info, "this={1} replicaId={2} - Replication operation stream completed", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_0_OperationDataFactory, 205, Error, "this={0} - Failed (oom)", "object"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Info_10_StatefulServiceGroupReplicationProcessing, 206, Info, "this={1} replicaId={2} - Waiting for atomic group state to be copied", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupReplicationProcessing, 207, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Stopping replication operation pump due to report fault", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Warning_3_StatefulServiceGroupCopyProcessing, 208, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Stopping copy operation pump due to report fault", "id", "object", "replicaId"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_4_OperationStream_StatefulServiceMember, 209, Error, "this={1} partitionId={2} - CreateEvent failed with {3}", "id", "object", "partitionId", "win32LastError"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_5_CopyContextStreamDispatcher, 210, Error, "this={0} - Could not deserialize copy context - error={1}", "object", "error"),
            SERVICEGROUPREPLICATION_STRUCTURED_TRACE(Error_6_StatefulServiceGroupReplicationProcessing, 211, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not deserialize replication operation = error={3}", "id", "object", "replicaId", "errorCode")
        {
        }

        static ServiceGroupReplicationEventSource const & GetEvents();

    };
}
