// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

TestDataLossHandler::SPtr TestDataLossHandler::Create(
    __in KAllocator & allocator,
    __in_opt KString const * const backupFolder,
    __in_opt FABRIC_RESTORE_POLICY restorePolicy)
{
    SPtr result = _new(TEST_DATALOSS_HANDLER_TAG, allocator) TestDataLossHandler(backupFolder, restorePolicy);
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

ktl::Awaitable<bool> TestDataLossHandler::DataLossAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (backupFolderPath_ == nullptr || this->IsValidFullBackupExist() == false)
    {
        co_return false;
    }

    ITransactionalReplicator::SPtr transactionalReplicator = GetTransactionalReplicator();

    status = co_await transactionalReplicator->RestoreAsync(*backupFolderPath_, restorePolicy_, timeout_, cancellationToken);
    THROW_ON_FAILURE(status);

    if (Common::Directory::Exists(static_cast<LPCWSTR>(*backupFolderPath_)))
    {
        Common::Directory::Delete(static_cast<LPCWSTR>(*backupFolderPath_), true);
    }

    co_return true;
}

bool TestDataLossHandler::IsValidFullBackupExist() const noexcept
{
    wstring fullMetadataFileName(FullMetadataFileName);
    vector<wstring> allFiles = Common::Directory::GetFiles(static_cast<LPCWSTR>(*backupFolderPath_), fullMetadataFileName, true, false);

    // IF size is 0, there is no full backup exist in the folder, if size more than 1, there are too many full backups.
    // Both of above cases are invalid.
    if (allFiles.size() == 1)
    {
        return true;
    }

    return false;
}

NTSTATUS TestDataLossHandler::Initialize(
    __in TxnReplicator::ITransactionalReplicator & replicator) noexcept
{
     NTSTATUS status = replicator.GetWeakIfRef(transactionalReplicatorWRef_);
     return status;
}

void TestDataLossHandler::SetBackupPath(
    __in_opt KString const * const backupFolderPath) noexcept
{
    backupFolderPath_ = backupFolderPath;
}

ITransactionalReplicator::SPtr TestDataLossHandler::GetTransactionalReplicator() noexcept
{
    ASSERT_IF(transactionalReplicatorWRef_ == nullptr, "Transactional Replicator WRef cannot be nullptr.");

    ITransactionalReplicator::SPtr result = transactionalReplicatorWRef_->TryGetTarget();
    ASSERT_IF(result == nullptr, "Transactional Replicator cannot be nullptr.");

    return result;
}

TestDataLossHandler::TestDataLossHandler(
    __in_opt KString const * const backupFolder,
    __in_opt FABRIC_RESTORE_POLICY restorePolicy)
    : KObject()
    , KShared()
    , backupFolderPath_(backupFolder)
    , restorePolicy_(restorePolicy)
    , timeout_(Common::TimeSpan::FromMinutes(30))
{
}

TestDataLossHandler::~TestDataLossHandler()
{
}
