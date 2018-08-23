// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestStateProviderManager final
        : public TxnReplicator::IStateProviderManager
        , public KObject<TestStateProviderManager>
        , public KShared<TestStateProviderManager>
        , public KWeakRefType<TestStateProviderManager>
    {
        K_FORCE_SHARED(TestStateProviderManager)
        K_SHARED_INTERFACE_IMP(IStateProviderManager)
        K_WEAKREF_INTERFACE_IMP(IStateProviderManager, TestStateProviderManager)

    public:

        static TestStateProviderManager::SPtr Create(
            __in ApiFaultUtility & apiFaultUtility,
            __in KAllocator & allocator);

        __declspec(property(get = get_StateStreamSuccessfulOperationCount, put = set_StateStreamSuccessfulOperationCount)) ULONG32 StateStreamSuccessfulOperationCount;
        ULONG32 get_StateStreamSuccessfulOperationCount() const
        {
            return stateStreamSuccessfulOperationCount_;
        }
        void set_StateStreamSuccessfulOperationCount(ULONG32 val)
        {
            stateStreamSuccessfulOperationCount_ = val;
        }

        __declspec(property(get = get_BeginSettingCurrentStateApiCount)) ULONG32 BeginSettingCurrentStateApiCount;
        ULONG32 get_BeginSettingCurrentStateApiCount() const
        {
            return beginSettingCurrentStateApiCount_;
        }

        __declspec(property(get = get_SetCurrentStateApiCount)) ULONG32 SetCurrentStateApiCount;
        ULONG32 get_SetCurrentStateApiCount() const
        {
            return setCurrentStateApiCount_;
        }

        __declspec(property(get = get_EndSettingCurrentStateApiCount)) ULONG32 EndSettingCurrentStateApiCount;
        ULONG32 get_EndSettingCurrentStateApiCount() const
        {
            return endSettingCurrentStateApiCount_;
        }

        ktl::Awaitable<NTSTATUS> ApplyAsync(
            __in LONG64 logicalSequenceNumber,
            __in TxnReplicator::TransactionBase const & transactionBase,
            __in TxnReplicator::ApplyContext::Enum applyContext,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const dataPtr,
            __out TxnReplicator::OperationContext::CSPtr & result) noexcept override;

        NTSTATUS Unlock(__in TxnReplicator::OperationContext const & operationContext) noexcept override;

        NTSTATUS PrepareCheckpoint(__in LONG64 checkpointLSN) noexcept override;

        ktl::Awaitable<NTSTATUS> PerformCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        ktl::Awaitable<NTSTATUS> CompleteCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        ktl::Awaitable<NTSTATUS> BackupCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        ktl::Awaitable<NTSTATUS> RestoreCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) noexcept override;

        NTSTATUS GetCurrentState(
            __in FABRIC_REPLICA_ID targetReplicaId,
            __out TxnReplicator::OperationDataStream::SPtr & result) noexcept override;

        ktl::Awaitable<NTSTATUS> BeginSettingCurrentStateAsync() noexcept override;
        
        ktl::Awaitable<NTSTATUS> EndSettingCurrentState() noexcept override;

        ktl::Awaitable<NTSTATUS> SetCurrentStateAsync(
            __in LONG64 stateRecordNumber,
            __in Data::Utilities::OperationData const & data) noexcept override;

        ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE newRole,
            __in ktl::CancellationToken & cancellationToken) override;

        void Reuse();
        
        void AddExpectedTransactionExceptionAtNthApply(
            __in LONG64 txId,
            __in ULONG nthApply,
            __in KSharedArray<Data::Utilities::OperationData::CSPtr> & expectedMetadata,
            __in KSharedArray<Data::Utilities::OperationData::CSPtr> & expectedApplyData);

        void AddExpectedTransactionApplyData(
            __in LONG64 txId,
            __in Data::Utilities::OperationData::CSPtr & expectedMetadata,
            __in Data::Utilities::OperationData::CSPtr & expectedApplyData);

        void AddExpectedTransactionApplyData(
            __in LONG64 txId,
            __in KSharedArray<Data::Utilities::OperationData::CSPtr> & expectedMetadata,
            __in KSharedArray<Data::Utilities::OperationData::CSPtr> & expectedApplyData);

        bool WaitForProcessingToComplete(
            __in LONG targetApplyCount,
            __in LONG targetUnlockCount,
            __in Common::TimeSpan const & timeout);

    private:

        struct ExpectedDataValue
        {
            KSharedArray<Data::Utilities::OperationData::CSPtr>::SPtr MetaData;
            KSharedArray<Data::Utilities::OperationData::CSPtr>::SPtr ApplyData;
            ULONG IndexToVerify;
            ULONG IndexToThrowException;
        };

        TestStateProviderManager(__in ApiFaultUtility & apiFaultUtility);

        ExpectedDataValue * GetValue(__in LONG64 txId);

        static void VerifyOperationData(
            __in_opt Data::Utilities::OperationData const * const actualData,
            __in_opt Data::Utilities::OperationData const * const expectedData);

        KSpinLock lock_;
        KHashTable<LONG64, ExpectedDataValue> expectedData_;
        Common::atomic_long applyCount_;
        Common::atomic_long unlockCount_;
        ApiFaultUtility::SPtr apiFaultUtility_;
        ULONG32 stateStreamSuccessfulOperationCount_;
        ULONG32 beginSettingCurrentStateApiCount_;
        ULONG32 setCurrentStateApiCount_;
        ULONG32 endSettingCurrentStateApiCount_;
    };
}

