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

void TestRestoreAsyncOperation::OnStart(
    __in Common::AsyncOperationSPtr const & proxySPtr)
{
    Common::Threadpool::Post([this, proxySPtr]()
    {
        DoRestore(proxySPtr);
    });
}

void TestRestoreAsyncOperation::DoRestore(
    __in Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = ErrorCodeValue::Success;

    try
    {
        SyncAwait(txnReplicator_.RestoreAsync(*backupFolderPath_, restorePolicy_, Common::TimeSpan::FromMinutes(5), ktl::CancellationToken::None));
    }
    catch (ktl::Exception & e)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(e.GetStatus()));
    }

    TryComplete(operation, error);
}

Common::ErrorCode TestRestoreAsyncOperation::End(
    __in Common::AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<TestRestoreAsyncOperation>(operation)->Error;
}
