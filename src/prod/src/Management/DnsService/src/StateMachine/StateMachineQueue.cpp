// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "StateMachineQueue.h"

/*static*/
void QueueItem::Create(
    __out QueueItem::SPtr& spItem,
    __in KAllocator& allocator,
    __in SmMessage message
)
{
    spItem = _new(TAG, allocator) QueueItem(message);
    KInvariant(spItem != nullptr);
}

QueueItem::QueueItem(
    __in SmMessage msg
) : _msg(msg)
{
}

QueueItem::~QueueItem()
{
}

/*static*/
void StateMachineQueue::Create(
    __out StateMachineQueue::SPtr& spQueue,
    __in KAllocator& allocator
)
{
    spQueue = _new(TAG, allocator) StateMachineQueue();
    KInvariant(spQueue != nullptr);
}

StateMachineQueue::StateMachineQueue() :
    SmQueue(TAG, FIELD_OFFSET(QueueItem, _links))
{
}

StateMachineQueue::~StateMachineQueue()
{
}

void StateMachineQueue::DeactivateQueue()
{
    if (_IsActive)
    {
        DroppedItemCallback callback(this, &StateMachineQueue::OnDroppedItem);
        SmQueue::Deactivate(callback);
    }
}

bool StateMachineQueue::Enqueue(
    __in QueueItem& item,
    __in Priority priority
)
{
    bool fResult = false;

    if ((priority != Priority::High) || (_EnqueuedItemLists[priority].Count() == 0))
    {
        item.AddRef();
        if (__super::Enqueue(item, static_cast<ULONG>(priority)) == STATUS_SUCCESS)
        {
            fResult = true;
        }
        else
        {
            item.Release();
        }
    }

    return fResult;
}

void StateMachineQueue::OnDroppedItem(
    __in SmQueue& deactivatingQueue,
    __in QueueItem& droppedItem
)
{
    UNREFERENCED_PARAMETER(deactivatingQueue);

    //
    // Undo AddRef that was done by the Enqueue.
    //
    droppedItem.Release();
}
