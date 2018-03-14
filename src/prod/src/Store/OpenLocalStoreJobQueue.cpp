// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Store;

StringLiteral const TraceComponent("OpenLocalStoreJobQueue");

//
// JobQueue singleton initialization
//

INIT_ONCE GlobalQueueInitOnce = INIT_ONCE_STATIC_INIT;
Global<OpenLocalStoreJobQueue> OpenLocalStoreJobQueue::GlobalQueue;

BOOL CALLBACK OpenLocalStoreJobQueue::CreateGlobalQueue(PINIT_ONCE, PVOID, PVOID *)
{
    if (!GlobalQueue)
    {
        auto root = make_shared<ComponentRoot>();
        GlobalQueue = Global<OpenLocalStoreJobQueue>(new OpenLocalStoreJobQueue(L"REStore.OpenLocalStoreJobQueue", *root));
    }

    return TRUE;
}

OpenLocalStoreJobQueue & OpenLocalStoreJobQueue::GetGlobalQueue()
{
    PVOID lpContext = NULL;
    BOOL success = ::InitOnceExecuteOnce(
        &GlobalQueueInitOnce,
        CreateGlobalQueue,
        NULL,
        &lpContext);

    ASSERT_IFNOT(success, "OpenLocalStoreJobQueue::CreateGlobalQueue failed");

    return *GlobalQueue;
}

//
// JobQueue
//

OpenLocalStoreJobQueue::OpenLocalStoreJobQueue(
    wstring const & name,
    __in ComponentRoot & root)
    : JobQueue(
        name, 
        root, 
        false, // forceEnqueue
        StoreConfig::GetConfig().OpenLocalStoreThrottle)
    , configHandlerId_(0)
{
    ComponentRootWPtr weakRoot = this->CreateComponentRoot();

    configHandlerId_ = StoreConfig::GetConfig().OpenLocalStoreThrottleEntry.AddHandler(
        [this, weakRoot](EventArgs const&)
        {
            auto root = weakRoot.lock();
            if (root)
            {
                this->OnConfigUpdate();
            }
        });
}

AsyncOperationSPtr OpenLocalStoreJobQueue::BeginEnqueueOpenLocalStore(
    __in ReplicatedStore & store,
    bool databaseShouldExist,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenLocalStoreAsyncOperation>(
        *this,
        store,
        databaseShouldExist,
        callback,
        parent);
}

ErrorCode OpenLocalStoreJobQueue::EndEnqueueOpenLocalStore(
    AsyncOperationSPtr const & operation)
{
    return OpenLocalStoreAsyncOperation::End(operation);
}
    
bool OpenLocalStoreJobQueue::TryEnqueue(__in ReplicatedStore & store, AsyncOperationSPtr const & operation)
{
    auto casted = dynamic_pointer_cast<OpenLocalStoreAsyncOperation>(operation);

    if (casted)
    {
        this->Enqueue(move(casted));

        return true;
    }
    else
    {
        TRACE_LEVEL_AND_TESTASSERT(store.WriteError, TraceComponent,
            "{0} dynamic_pointer_cast<OpenLocalStoreAsyncOperation> failed", 
            store.TraceId);

        return false;
    }
}

void OpenLocalStoreJobQueue::UnregisterAndClose()
{
    StoreConfig::GetConfig().OpenLocalStoreThrottleEntry.RemoveHandler(configHandlerId_);

    JobQueue::Close();
}

void OpenLocalStoreJobQueue::OnConfigUpdate()
{
    this->UpdateMaxThreads(StoreConfig::GetConfig().OpenLocalStoreThrottle);
}

//
// JobItem
//

OpenLocalStoreAsyncOperation::OpenLocalStoreAsyncOperation(
    __in OpenLocalStoreJobQueue & jobQueue,
    __in ReplicatedStore & store,
    bool databaseShouldExist,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , jobQueue_(jobQueue)
    , store_(store)
    , databaseShouldExist_(databaseShouldExist)
{
}

ErrorCode OpenLocalStoreAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<OpenLocalStoreAsyncOperation>(operation)->Error;
}

void OpenLocalStoreAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    store_.WriteInfo(
        TraceComponent, 
        "{0} queue local store open job: throttle={1} queued={2}",
        store_.TraceId,
        StoreConfig::GetConfig().OpenLocalStoreThrottle,
        jobQueue_.GetQueueLength());
        
    if (!jobQueue_.TryEnqueue(store_, thisSPtr))
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
    }
}

void OpenLocalStoreAsyncOperation::OnCancel()
{
    store_.WriteInfo(
        TraceComponent, 
        "{0} cancel local store open job",
        store_.TraceId);
}

bool OpenLocalStoreAsyncOperation::ProcessJob(ComponentRoot &)
{
    // Handle race with open cancel from RA since the local store 
    // lifetime must remain valid during initialization and
    // cancelling the open results in calling ReleaseLocalStore().
    //
    if (this->TryStartComplete())
    {
        store_.WriteInfo(
            TraceComponent, 
            "{0} processing local store open job",
            store_.TraceId);
        
        auto error = store_.InitializeLocalStoreForJobQueue(databaseShouldExist_);

        if (this->IsCancelRequested)
        {
            error = ErrorCodeValue::OperationCanceled;
        }

        this->FinishComplete(shared_from_this(), error);
    }
    else
    {
        store_.WriteInfo(
            TraceComponent, 
            "{0} skip local store open job",
            store_.TraceId);
    }

    return true;
}
