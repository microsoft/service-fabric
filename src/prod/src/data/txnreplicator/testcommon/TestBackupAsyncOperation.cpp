// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;
using namespace Common;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

ErrorCode TestBackupAsyncOperation::End(
    __in AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<TestBackupAsyncOperation>(operation)->Error;
}

void TestBackupAsyncOperation::OnStart(
    __in AsyncOperationSPtr const & proxySPtr)
{
    DoBackup(proxySPtr);
}

void TestBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

ktl::Task TestBackupAsyncOperation::DoBackup(
    __in AsyncOperationSPtr const & operation) noexcept
{
    AsyncOperationSPtr const snap = operation;
    BackupInfo result;
    NTSTATUS status = co_await txnReplicator_.BackupAsync(*backupCallbackHandler_, backupOption_, Common::TimeSpan::FromMinutes(5), ktl::CancellationToken::None, result);
    ErrorCode error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));

    TryComplete(snap, error);
    co_return;
}
