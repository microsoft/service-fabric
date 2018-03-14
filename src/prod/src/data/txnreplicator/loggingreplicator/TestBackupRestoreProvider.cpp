// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

TestBackupRestoreProvider::SPtr TestBackupRestoreProvider::Create(
    __in KAllocator& allocator) noexcept
{
    TestBackupRestoreProvider * result = _new(TEST_BACKUP_RESTORE_PROVIDER_TAG, allocator) TestBackupRestoreProvider();

    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return SPtr(result);
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::CancelTheAbortingOfTransactionsIfNecessaryAsync() noexcept
{
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::OpenAsync(
    __in_opt TxnReplicator::IBackupFolderInfo const * const backupFolderInfoPtr,
    __out TxnReplicator::RecoveryInformation & recoveryInformation) noexcept
{
    UNREFERENCED_PARAMETER(backupFolderInfoPtr);
    UNREFERENCED_PARAMETER(recoveryInformation);
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::PerformRecoveryAsync(__in TxnReplicator::RecoveryInformation const & recoveryInfo) noexcept
{
    UNREFERENCED_PARAMETER(recoveryInfo);
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::BecomePrimaryAsync(__in bool isForRestore) noexcept
{
    UNREFERENCED_PARAMETER(isForRestore);
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::CloseAsync() noexcept
{
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::PrepareForRestoreAsync() noexcept
{
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> TestBackupRestoreProvider::PerformUpdateEpochAsync(
    __in TxnReplicator::Epoch const & epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastLsn) noexcept
{
    UNREFERENCED_PARAMETER(epoch);
    UNREFERENCED_PARAMETER(previousEpochLastLsn);

    co_return STATUS_SUCCESS;
}

TestBackupRestoreProvider::TestBackupRestoreProvider()
    : KObject()
    , KShared()
    , IBackupRestoreProvider()
{
}

TestBackupRestoreProvider::~TestBackupRestoreProvider()
{
}
