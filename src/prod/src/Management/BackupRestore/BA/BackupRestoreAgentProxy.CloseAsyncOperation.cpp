// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace BackupRestoreAgentComponent;

void BackupRestoreAgentProxy::CloseAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    owner_.ipcClient_.UnregisterMessageHandler(Transport::Actor::BA);

    if ((--outstandingOperations_) == 0)
    {
        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}

ErrorCode BackupRestoreAgentProxy::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<BackupRestoreAgentProxy::CloseAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}
