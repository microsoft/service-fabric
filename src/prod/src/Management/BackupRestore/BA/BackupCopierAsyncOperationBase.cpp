// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

BackupCopierAsyncOperationBase::BackupCopierAsyncOperationBase(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent, true) // control our own completion on Cancel
    , owner_(owner)
    , traceId_(dynamic_cast<Federation::NodeTraceComponent<Common::TraceTaskCodes::BA>&>(owner).TraceId)
    , activityId_(activityId)
    , timeoutHelper_(timeout)
    , jobCompletionCallback_(nullptr)
{
}

void BackupCopierAsyncOperationBase::OnStart(AsyncOperationSPtr const &)
{
    // Intentional no-op
}

void BackupCopierAsyncOperationBase::OnCompleted()
{
    if (jobCompletionCallback_)
    {
        jobCompletionCallback_();

        jobCompletionCallback_ = nullptr;
    }
}

void BackupCopierAsyncOperationBase::SetJobQueueItemCompletion(JobQueueItemCompletionCallback const & callback)
{
    jobCompletionCallback_ = callback;
}
