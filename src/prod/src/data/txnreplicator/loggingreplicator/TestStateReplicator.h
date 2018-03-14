// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestStateReplicator final
        : public Data::LoggingReplicator::IStateReplicator
        , public KObject<TestStateReplicator>
        , public KShared<TestStateReplicator>
    {
        K_FORCE_SHARED(TestStateReplicator)
        K_SHARED_INTERFACE_IMP(IStateReplicator)
 
    public:

        static TestStateReplicator::SPtr Create(
            __in ApiFaultUtility & apiFaultUtility,
            __in KAllocator & allocator);

        __declspec(property(get = get_CurrentStateReplicatorLsn, put = set_CurrentStateReplicatorLsn)) LONG64 LSN;
        LONG64 get_CurrentStateReplicatorLsn()
        {
            return lsn_.load();
        }
        void set_CurrentStateReplicatorLsn(LONG64 newLsn)
        {
            lsn_.store(newLsn);
        }

        __declspec(property(get = get_CopyStreamMaximumSuccessfulOperationCount, put = set_CopyStreamMaximumSuccessfulOperationCount)) ULONG32 CopyStreamMaximumSuccessfulOperationCount;
        ULONG32 get_CopyStreamMaximumSuccessfulOperationCount()
        {
            return copyStreamMaximumSuccessfulOperationCount_;
        }
        void set_CopyStreamMaximumSuccessfulOperationCount(ULONG32 value)
        {
            copyStreamMaximumSuccessfulOperationCount_ = value;
        }

        __declspec(property(get = get_ReplicationStreamMaximumSuccessfulOperationCount, put = set_ReplicationStreamMaximumSuccessfulOperationCount)) ULONG32 ReplicationStreamMaximumSuccessfulOperationCount;
        ULONG32 get_ReplicationStreamMaximumSuccessfulOperationCount()
        {
            return replicationSteramMaximumSuccessfulOperationCount_;
        }
        void set_ReplicationStreamMaximumSuccessfulOperationCount(ULONG32 value)
        {
            replicationSteramMaximumSuccessfulOperationCount_ = value;
        }

        TxnReplicator::CompletionTask::SPtr ReplicateAsync(
            __in Data::Utilities::OperationData const & replicationData,
            __out LONG64 & logicalSequenceNumber) override;

        void FailReplicationAfterLSN(__in LONG64 lsn, __in NTSTATUS e = STATUS_CANCELLED);

        void SetBeginReplicateException(__in NTSTATUS e);

        void InitializePrimaryCopyStream(__in Data::LoggingReplicator::CopyStream & copyStream);

        NTSTATUS GetCopyStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result) override;
        
        NTSTATUS GetReplicationStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result) override;

        int64 GetMaxReplicationMessageSize() override;

    private:

        TestStateReplicator(__in ApiFaultUtility & apiFaultUtility);

        NTSTATUS ReturnErrorAfterLsnIfNeeded(__in uint64 lsn);

        ApiFaultUtility::SPtr apiFaultUtility_;
        Common::atomic_uint64 lsn_;
        Common::atomic_uint64 failReplicateAfterLsn_;
        Data::LoggingReplicator::CopyStream::SPtr copyStream_;
        NTSTATUS beginReplicateException_;
        NTSTATUS endReplicateException_;
        ULONG32 copyStreamMaximumSuccessfulOperationCount_;
        ULONG32 replicationSteramMaximumSuccessfulOperationCount_;
    };
}
