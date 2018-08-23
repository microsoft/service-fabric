// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class TestStateProvider2
        : public KObject<TestStateProvider2>
        , public KShared<TestStateProvider2>
        , public TxnReplicator::IStateProvider2
    {
        K_FORCE_SHARED(TestStateProvider2)
        K_SHARED_INTERFACE_IMP(IStateProvider2)

    public:
        static TestStateProvider2::SPtr Create(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in TxnReplicator::TestCommon::TestStateProvider & innerStateProvider,
            __in KAllocator & allocator);

        // Test APIs
        ktl::Awaitable<void> GetAsync(
            __in Common::TimeSpan const & timeout,
            __out FABRIC_SEQUENCE_NUMBER & version,
            __out KString::SPtr & value);

        ktl::Awaitable<bool> TryUpdateAsync(
            __in Common::TimeSpan const & timeout,
            __in TxnReplicator::Transaction & transaction,
            __in KStringView const & newValue,
            __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
            __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

        ktl::Awaitable<bool> TryRemoveMyself(
            __in Common::TimeSpan const & timeout,
            __in TxnReplicator::Transaction & transaction,
            __in TxnReplicator::ITransactionalReplicator & replicator,
            __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
            __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

        // IStateProvider APIs.
        KUriView const GetName() const override;

        KArray<TxnReplicator::StateProviderInfo> GetChildren(__in KUriView const & rootName) override;

        VOID Initialize(
            __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
            __in KStringView const & workFolder,
            __in_opt KSharedArray<IStateProvider2::SPtr> const * const children) override;

        ktl::Awaitable<void> OpenAsync(__in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE newRole,
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;
        void Abort() noexcept override;

        void PrepareCheckpoint(__in LONG64 checkpointLSN) override;

        ktl::Awaitable<void> PerformCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> CompleteCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> RecoverCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> BackupCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> RestoreCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> RemoveStateAsync(
            __in ktl::CancellationToken const & cancellationToken) override;

        TxnReplicator::OperationDataStream::SPtr GetCurrentState() override;

        ktl::Awaitable<void> BeginSettingCurrentStateAsync() override;

        ktl::Awaitable<void> SetCurrentStateAsync(
            __in LONG64 stateRecordNumber,
            __in Data::Utilities::OperationData const & operationData,
            __in ktl::CancellationToken const& cancellationToken) override;

        ktl::Awaitable<void> EndSettingCurrentStateAsync(
            __in ktl::CancellationToken const& cancellationToken) override;

        ktl::Awaitable<void> PrepareForRemoveAsync(
            __in TxnReplicator::TransactionBase const & transactionBase,
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAsync(
            __in LONG64 logicalSequenceNumber,
            __in TxnReplicator::TransactionBase const & transactionBase,
            __in TxnReplicator::ApplyContext::Enum applyContext,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const dataPtr) override;

        void Unlock(__in TxnReplicator::OperationContext const & operationContext) override;

        void WaitForSignalReset(std::wstring const & strSignal) const
        {
            if (TestHooks::ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, strSignal))
            {
                TestHooks::ApiSignalHelper::WaitForSignalReset(nodeId_, serviceName_, strSignal);
            }
        }

    private:
        NOFAIL TestStateProvider2(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in TxnReplicator::TestCommon::TestStateProvider & innerStateProvider) noexcept;

        ktl::Awaitable<void> CheckForFailuresAndDelaysAsync(
            std::wstring const & operationName,
            bool isHighFrequencyApi) const;

        void CheckForFailuresAndDelays(
            std::wstring const & operationName,
            bool isHighFrequencyApi) const;

        bool ShouldFailOn(
            std::wstring operationName,
            ApiFaultHelper::FaultType faultType,
            bool isHighFrequencyApi) const;
        
        TxnReplicator::TestCommon::TestStateProvider::SPtr const innerStateProvider_;
        std::wstring const nodeId_;
        std::wstring const serviceName_;
    };
}
