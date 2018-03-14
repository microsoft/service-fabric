// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Store
{
    Common::StringLiteral const TraceComponent("RestoreAsyncOperation");

    void ReplicatedStore::RestoreAsyncOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        Common::Threadpool::Post([this, proxySPtr]()
        {			
            DoRestore(proxySPtr);
        });
    }

    void ReplicatedStore::RestoreAsyncOperation::DoRestore(Common::AsyncOperationSPtr const & operation)
    {				
        auto error = owner_.RestoreLocal(backupDir_, restoreSettings_);

        TryComplete(operation, error);
    }

    Common::ErrorCode ReplicatedStore::RestoreAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<RestoreAsyncOperation>(operation);
        return casted->Error;
    }
}
