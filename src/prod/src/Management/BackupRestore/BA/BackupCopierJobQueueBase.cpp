// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

BackupCopierJobQueueBase::BackupCopierJobQueueBase(
    wstring const & name,
    __in ComponentRoot & root)
    : JobQueue(
        name,
        root,
        false, // forceEnqueue
        0) // maxThreads
    , configHandlerId_(0)
{
    this->SetAsyncJobs(true);
}

void BackupCopierJobQueueBase::TryEnqueueOrFail(__in BackupCopierProxy & owner, AsyncOperationSPtr const & operation)
{
    // BackupCopierAsyncOperationBase::OnStart() is a no-op. No work is performed
    // (AsyncOperation can't complete) until it's processed by the job queue.
    //
    operation->Start(operation);

    auto casted = dynamic_pointer_cast<BackupCopierAsyncOperationBase>(operation);

    if (casted)
    {
        auto selfRoot = operation->Parent;

        casted->SetJobQueueItemCompletion([this, selfRoot] { this->OnJobQueueItemComplete(); });

        this->Enqueue(BackupCopierJobItem(move(casted)));
    }
    else
    {
        auto msg = wformatString(
            "{0} dynamic_pointer_cast<BackupCopierAsyncOperationBase> failed",
            dynamic_cast<Federation::NodeTraceComponent<Common::TraceTaskCodes::BA>&>(owner).TraceId);

        owner.WriteError(TraceComponent, "{0}", msg);

        Assert::TestAssert("{0}", msg);

        operation->TryComplete(operation, ErrorCodeValue::OperationFailed);
    }
}

void BackupCopierJobQueueBase::UnregisterAndClose()
{
    this->GetThrottleConfigEntry().RemoveHandler(configHandlerId_);

    JobQueue::Close();
}

void BackupCopierJobQueueBase::InitializeConfigUpdates()
{
    this->OnConfigUpdate();

    ComponentRootWPtr weakRoot = this->CreateComponentRoot();

    configHandlerId_ = this->GetThrottleConfigEntry().AddHandler(
        [this, weakRoot](EventArgs const&)
    {
        auto root = weakRoot.lock();
        if (root)
        {
            this->OnConfigUpdate();
        }
    });
}

void BackupCopierJobQueueBase::OnConfigUpdate()
{
    this->UpdateMaxThreads(this->GetThrottleConfigValue());
}

void BackupCopierJobQueueBase::OnJobQueueItemComplete()
{
    this->CompleteAsyncJob();
}
