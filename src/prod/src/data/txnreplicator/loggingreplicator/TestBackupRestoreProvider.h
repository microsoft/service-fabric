// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestBackupRestoreProvider
        : public KObject<TestBackupRestoreProvider>
        , public KShared<TestBackupRestoreProvider>
        , public KWeakRefType<TestBackupRestoreProvider>
        , public TxnReplicator::IBackupRestoreProvider
    {
        K_FORCE_SHARED(TestBackupRestoreProvider);
        K_SHARED_INTERFACE_IMP(IBackupRestoreProvider);
        K_WEAKREF_INTERFACE_IMP(IBackupRestoreProvider, TestBackupRestoreProvider);

    public: // Statics
        static SPtr Create(
            __in KAllocator & allocator) noexcept;

    public:
        ktl::Awaitable<NTSTATUS> CancelTheAbortingOfTransactionsIfNecessaryAsync() noexcept override;

        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in_opt TxnReplicator::IBackupFolderInfo const * const backupFolderInfoPtr,
            __out TxnReplicator::RecoveryInformation & recoveryInformation) noexcept override;
        
        ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(__in TxnReplicator::RecoveryInformation const & recoveryInfo) noexcept override;
        
        ktl::Awaitable<NTSTATUS> BecomePrimaryAsync(__in bool isForRestore) noexcept override;
        
        ktl::Awaitable<NTSTATUS> CloseAsync() noexcept override;
        
        ktl::Awaitable<NTSTATUS> PrepareForRestoreAsync() noexcept override;
        
        ktl::Awaitable<NTSTATUS> PerformUpdateEpochAsync(
                __in TxnReplicator::Epoch const & epoch,
                __in FABRIC_SEQUENCE_NUMBER previousEpochLastLsn) noexcept override;
    };
}
