// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

StateMachineBase::StateMachineBase() :
    _currentState(StartStateIndex)
{
    StateMachineQueue::Create(/*out*/_spQueue, GetThisAllocator());
}

StateMachineBase::~StateMachineBase()
{
}

void StateMachineBase::ActivateStateMachine()
{
    _currentState = StartStateIndex;

    _spQueue->Reuse();
    KAsyncContextBase::CompletionCallback queueCallback(this, &StateMachineBase::OnStateMachineQueueCompleted);
    if (STATUS_PENDING != _spQueue->Activate(this, queueCallback))
    {
        KInvariant(false);
    }

    _spDequeueOp = nullptr;
    if (!NT_SUCCESS(_spQueue->CreateDequeueOperation(/*out*/_spDequeueOp)))
    {
        KInvariant(false);
    }

    _spDequeueOp->Reuse();
    KAsyncContextBase::CompletionCallback opCallback(this, &StateMachineBase::OnDequeue);
    _spDequeueOp->StartDequeue(this, opCallback);

    OnStateEnter_Start();
}

void StateMachineBase::DeactivateStateMachine()
{
    _spQueue->DeactivateQueue();
}

void StateMachineBase::ChangeStateAsync(
    __in bool fSuccess
)
{
    QueueItem::SPtr spItem;
    QueueItem::Create(/*out*/spItem, GetThisAllocator(), fSuccess ? SmMsgSuccess : SmMsgFailure);
    _spQueue->Enqueue(*spItem, Priority::Normal);
}

void StateMachineBase::TerminateAsync()
{
    QueueItem::SPtr spItem;
    QueueItem::Create(/*out*/spItem, GetThisAllocator(), SmMsgTerminate);
    _spQueue->Enqueue(*spItem, Priority::High);
}

void StateMachineBase::OnDequeue(
    __in_opt KAsyncContextBase* const pParentContext,
    __in KAsyncContextBase& completedContext
)
{
    UNREFERENCED_PARAMETER(pParentContext);

    SmQueue::DequeueOperation* pDequeueOp = static_cast<SmQueue::DequeueOperation*>(&completedContext);

    if ((completedContext.Status() == K_STATUS_SHUTDOWN_PENDING) || (completedContext.Status() == STATUS_CANCELLED))
    {
        return;
    }

    QueueItem& item = pDequeueOp->GetDequeuedItem();
    QueueItem::SPtr spItem;
    spItem.Attach(&item);

    ChangeStateInternal(item._msg);

    if (_currentState != EndStateIndex)
    {
        _spDequeueOp->Reuse();
        KAsyncContextBase::CompletionCallback callback(this, &StateMachineBase::OnDequeue);
        _spDequeueOp->StartDequeue(this, callback);
    }
}

void StateMachineBase::OnStateMachineQueueCompleted(
    __in_opt KAsyncContextBase* const pParentContext,
    __in KAsyncContextBase& completedContext
)
{
    UNREFERENCED_PARAMETER(pParentContext);
    UNREFERENCED_PARAMETER(completedContext);
}
