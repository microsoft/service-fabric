// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

using namespace std;
using namespace ktl;
using namespace Common;
using namespace TxnReplicator;

TestBackupCallbackHandler::SPtr TestBackupCallbackHandler::Create(
    __in KString const & externalBackupFolderPath,
    __in KAllocator & allocator)
{
    TestBackupCallbackHandler * result = _new(TEST_BACKUP_RESTORE_PROVIDER_TAG, allocator) TestBackupCallbackHandler(externalBackupFolderPath);

    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return SPtr(result);
}

ktl::Awaitable<bool> TestBackupCallbackHandler::UploadBackupAsync(
    __in BackupInfo backupInfo,
    __in CancellationToken const & cancellationToken)
{
    NTSTATUS status = backupInfoArray_.Append(backupInfo);
    THROW_ON_FAILURE(status);

    wstring backupDirectory(backupInfo.BackupId.ToString());
    KStringView backupIdString(backupDirectory.c_str());
    KString::SPtr storeFolderSPtr = Data::Utilities::KPath::Combine(*externalBackupFolderPath_, backupIdString, GetThisAllocator());

    ErrorCode errorCode = Directory::Copy(wstring(*backupInfo.Directory), wstring(*storeFolderSPtr), false);
    ASSERT_IFNOT(errorCode.IsSuccess(), "Directory copy failed with {0}", errorCode.ToHResult());

    co_return true;
}

KArray<TxnReplicator::BackupInfo> TestBackupCallbackHandler::get_BackupInfoArray() const
{
    return backupInfoArray_;
}

TestBackupCallbackHandler::TestBackupCallbackHandler(
    __in KString const & externalBackupFolderPath)
    : externalBackupFolderPath_(&externalBackupFolderPath)
    , backupInfoArray_(GetThisAllocator())
{
    SetConstructorStatus(backupInfoArray_.Status());
}

TestBackupCallbackHandler::~TestBackupCallbackHandler()
{
}
