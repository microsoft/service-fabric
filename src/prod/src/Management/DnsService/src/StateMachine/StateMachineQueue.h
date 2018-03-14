// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    enum Priority
    {
        High = 0,
        Normal,
        Count
    };

    enum SmMessage : ULONG
    {
        SmMsgSuccess = 1,
        SmMsgFailure,
        SmMsgTerminate
    };

    class QueueItem :
        public KShared<QueueItem>
    {
        K_FORCE_SHARED(QueueItem);

    public:
        static void Create(
            __out QueueItem::SPtr& spItem,
            __in KAllocator& allocator,
            __in SmMessage message
        );

    private:
        QueueItem(
            __in SmMessage msg
        );

    public:
        KListEntry _links;
        SmMessage  _msg;
    };

    typedef KAsyncQueue<QueueItem, Priority::Count> SmQueue;

    class StateMachineQueue : public SmQueue
    {
        K_FORCE_SHARED_WITH_INHERITANCE(StateMachineQueue);

    private:
        //
        // Hide some of KAsyncQueue's public interface
        //
        using KAsyncQueue::Create;
        using KAsyncQueue::Enqueue;
        using KAsyncQueue::Deactivate;

    public:
        static void Create(
            __out StateMachineQueue::SPtr& spQueue,
            __in KAllocator& allocator
        );

        void DeactivateQueue();

        virtual bool Enqueue(
            __in QueueItem& message,
            __in Priority priority
        );

    private:
        void OnDroppedItem(
            __in SmQueue& deactivatingQueue,
            __in QueueItem& droppedItem
        );
    };
}
