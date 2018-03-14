// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IBackupCallbackHandler
    {
        K_SHARED_INTERFACE(IBackupCallbackHandler)

    public:
        virtual ktl::Awaitable<bool> UploadBackupAsync(
            __in BackupInfo backupInfo,
            __in ktl::CancellationToken const & cancellationToken) = 0;
    };
}
