// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IBackupRestoreProvider
    {
        K_SHARED_INTERFACE(IBackupRestoreProvider);
        K_WEAKREF_INTERFACE(IBackupRestoreProvider);

    public:
        virtual ktl::Awaitable<NTSTATUS> CancelTheAbortingOfTransactionsIfNecessaryAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> OpenAsync(
            __in_opt IBackupFolderInfo const * const backupFolderInfoPtr,
            __out RecoveryInformation & recoveryInformation) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> PerformRecoveryAsync(__in TxnReplicator::RecoveryInformation const & recoveryInfo) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> BecomePrimaryAsync(__in bool isForRestore) noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> CloseAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> PrepareForRestoreAsync() noexcept = 0;

        virtual ktl::Awaitable<NTSTATUS> PerformUpdateEpochAsync(
            __in TxnReplicator::Epoch const & epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastLsn) noexcept = 0;
    };
}
