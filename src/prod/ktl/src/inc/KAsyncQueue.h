/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KAsyncQueue.h

    Description:
      Kernel Template Library (KTL): Async Queue Support Definitions

    History:
      richhas          16-Feb-2011         Initial version.

--*/

#pragma once

//** KAsyncQueue class
//
// Description:
//      This template is used to generate a strongly typed queue class for items of type IType. It is a Service
//      in its own right and as such follows the Activate()/Deactivate() pattern. The is an optional template
//      parameter (NPriorities) that can be used to specify the number of priorities supported by Enqueue().
//
//      Any items that have been queued before Deactivate() is called will be processed normally (drained)
//      by default. However optional behavior is provided during deactivation where any queued items at that
//      time are dropped but passed thru a KDelegate provided to Deactivate().
//
//      Enqueuing IType instances is accomplished in sync fashion via Enqueue(). Dequeuing is accomplished
//      in an async fashion using instances of DequeueOperation (a KAsyncContext) and its StartDequeue() method.
//
//      Any number of consumers (DequeueOperation instances) can be started. They Complete when either there
//      is a corresponding IType instance enqueued, the hosting KAsyncQueue is being deactivated, or a given
//      DequeueOperation instance itself is Cancel()'d. These conditions are indicated with a corresponding
//      DequeueOperation completion Status() of STATUS_SUCCESS, K_STATUS_SHUTDOWN_PENDING, or STATUS_CANCELLED.
//      Once the in a Deactivating state, any items that have been queued before Deactivate() is called will be
//      processed normally (drained) or dropped and any pending DequeueOperations will Complete with a Status() of
//      K_STATUS_SHUTDOWN_PENDING.
//
//      KAsyncQueue and DequeueOperation instances may both be Reuse()'d. A KAsyncQueue instance may be Reuse()'d
//      only after Deactivation on that instance has finished - each Activate/Deactivation cycle causes a new
//      version of that KAsyncQueue to be established. A DequeueOperation instance may only be reused within the
//      same KAsyncQueue version that it was created under.
//
template<class IType, ULONG NPriorities = 1>
class KAsyncQueue : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncQueue);

public:
    //* Factory for creation of KAsyncQueue Service instances.
    //
    //  Parameters:
    //      AllocationTag, Allocator        - Standard memory allocation component and tag
    //      ITypeLinkOffset                 - FIELD_OFFSET(IType, Links) - where Links is a KListEntry
    //                                        data member in IType to be used for queuing instances
    //      Context                         - SPtr that points to an allocated KAsyncQueue<IType> instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated KAsyncQueue<IType> instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in ULONG const ITypeLinkOffset,
        __out KSharedPtr<KAsyncQueue<IType>>& Context);


    //* Delegate for processing dropped IType instances during deactivation
    //
    typedef KDelegate<VOID(
        KAsyncQueue<IType, NPriorities>&     DeactivatingQueue,
        IType&                  DroppedItem
    )> DroppedItemCallback;


    //* Activate an instance of a KAsyncQueue<> Service.
    //
    // KAsyncQueue Service instances that are useful are an KAsyncContext in the operational state.
    // This means that the supplied callback (Callback) will be invoked once Deactivate() is called
    // and the instance has finished its shutdown process - meaning any queued item have been dispatched
    // for processing by a DequeueOperation instance - meaning the completion process started on the
    // corresponding DequeueOperation. NOTE: This means that the final successful DequeueOperation completion
    // can race with the completion of the owning KAsyncQueue triggered by Deactivate().
    //
    // Returns: STATUS_PENDING - Service successful started. Once this status is returned, that
    //                           instance will accept other operations until Deactive() is called.
    //
    NTSTATUS
    Activate(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback Callback);


    //* Trigger shutdown on an active KAsyncQueue<> Service instance.
    //
    // Once called, a Deactivated (or deactivating) KAsyncQueue will reject any further operations with
    // a Status or result of K_STATUS_SHUTDOWN_PENDING. Any existing queued items will be drained via
    // DequeueOperation completions unless an optional DroppedItemCallback is provided. If such a callback
    // is provided, that callback will be called for any and all IType items that are still queued at that
    // time BEFORE completion occurs on the KAsyncQueue.
    //
    // Parameters:
    //      Callback - optional callback that is invoked for each dropped IType item.
    //
    VOID
    Deactivate(__in_opt DroppedItemCallback Callback = nullptr);


    //* Enqueue an IType instance for async processing
    //
    //  Parameters:
    //      Priority                    - Indicates the priority of the enqueued IType. Valid values
    //                                    are 0-(NPriorities - 1); with 0 being the highest in that
    //                                    items at priority 0 will be dequeued before any items at
    //                                    lower level priorities.
    //
    // Returns:
    //      STATUS_SUCCESS              - Item is queued
    //      K_STATUS_SHUTDOWN_PENDING   - Queue is in the process of shutting down
    //
    NTSTATUS
    Enqueue(IType& ItemToQueue, ULONG Priority = (NPriorities - 1));

    //* Attempt to cancel a previously enqueued IType instance
    //
    //  Parameters:
    //      ItemToCancel - Reference to the IType to attempt to cancel
    //
    //  Returns:
    //      TRUE        - ItemToCancel was removed from the queue before it was dispatched
    //      FALSE       - ItemToCancel was (or will) be processed - the CancelEnqueue() call
    //                    lost the race with the queue's dequeuing facility OR the KAsyncQueue
    //                    instance is in the process of being shutdown.
    //
    BOOLEAN
    CancelEnqueue(IType& ItemToCancel);

    //* Drop any enqueued items; calling a caller provided callback for each item dropped
    //
    //	Can be used with Deactivate() to force drop any unconsumed items allowing the two
    //  forms of Deactivate() to be differienated
    //
    //
    // Parameters:
    //      Callback - callback that is invoked for each dropped IType item.
    //
    //  Returns:
    //      STATUS_SUCCESS                  - All items dropped
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    NTSTATUS
    CancelAllEnqueued(__in_opt DroppedItemCallback Callback);

    //* Pauses dequeue operations.
    //
    //  Dequeue operations waiting in the queue are not processed while dequeue is in paused state.
    //  Dequeue operations that are posted while dequeue is paused are added to the waiting list.
    //
    VOID
    PauseDequeue();

    //* Resumes dequeue operations.
    //
    //  Dequeue operations from the waiting list are processed until there are no more items in
    //  the queue or until there are no more waiting dequeue operations.
    //
    VOID
    ResumeDequeue();

    //* Async dequeue operation
    class DequeueOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(DequeueOperation);

    public:
        // Post a Async Dequeue Operation
        //
        // Parameters:  Std KAsyncContext arrangement
        //
        // When the completion callback is called the Status() of the completing
        // DequeueOperation instance will indicate if GetDequeuedItem() will return a reference
        // to an instance of IType for processing - STATUS_SUCCESS. If Status() returns
        // K_STATUS_SHUTDOWN_PENDING, the KAsyncQueue is being deactivated. If STATUS_CANCELLED
        // is return from Status(), Cancel() was called.
        //
        VOID
        StartDequeue(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        #if defined(K_UseResumable)
        // Start an awaitable DequeueOperation operation
        //
        // Parameters:
        //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
        //
        //	Usage: 
        //			NTSTATUS status = co_await myGate->StartAcquireAsync(nullptr)
        //
        ktl::Awaitable<NTSTATUS> StartDequeueAsync(
            __in_opt KAsyncContextBase* const Parent) noexcept
        {
            ktl::kasync::StartAwaiter::SPtr awaiter;

            NTSTATUS status = ktl::kasync::StartAwaiter::Create(
                GetThisAllocator(),
                GetThisAllocationTag(),
                *this,
                awaiter,
                Parent,
                nullptr);

            if (!NT_SUCCESS(status))
            {
                co_return status;
            }

            status = co_await *awaiter;
            co_return status;
        }
        #endif

        // Accessor method for the dequeued IType instance reference - must only be
        // called when Status() == STATUS_SUCCESS.
        IType& GetDequeuedItem();

    protected:
        friend class KAsyncQueue;

        DequeueOperation(KAsyncQueue<IType, NPriorities>& Owner, ULONG const OwnerVersion);

        VOID
        OnStart();

        VOID
        OnReuse();

        VOID
        OnCancel();

        virtual VOID
        CompleteDequeue(IType& ItemDequeued);

        virtual VOID
        CompleteDequeue(NTSTATUS Status);

    protected:
        KSharedPtr<KAsyncQueue<IType, NPriorities>> _Owner;
        ULONG                                   _OwnerVersion;
        IType*                                  _Result;
        KListEntry                              _Links;
    };

    //* DequeueOperation Factory
    //
    //  Parameters:
    //      Context                         - SPtr that points to the allocated DequeueOperation instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated DequeueOperation instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    //  NOTE: Instances of DequeueOperation created by this factory are bound to the producing KAsyncQueue
    //        instance AND the specific Activation-version of that KAsyncQueue and may only be used with
    //        that binding.
    //
    NTSTATUS
    CreateDequeueOperation(__out KSharedPtr<DequeueOperation>& Context);

    //* Test support methods
    ULONG
    GetNumberOfQueuedItems();

    ULONG
    GetNumberOfWaiters();

    #if defined(K_UseResumable)
    // Start an awaitable DequeueOperation operation
    //
    // Parameters:
    //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
    //
    //	Usage: 
    //			NTSTATUS status = co_await myGate->StartAcquireAsync(nullptr)
    //
    ktl::Awaitable<NTSTATUS> StartDequeueAsync(
        __out IType& Result,
        __in_opt KAsyncContextBase* const Parent) noexcept
    {
        typename DequeueOperation::SPtr  dqEntry;

        NTSTATUS status = CreateDequeueOperation(dqEntry);
        if (NT_SUCCESS(status))
        {
            status = co_await dqEntry->StartDequeueAsync(Parent);
            if (NT_SUCCESS(status))
            {
                Result = dqEntry->GetDequeuedItem();
            }
        }
        co_return{ status };
    }
    #endif



protected:
    //* Derivation support interface
    KAsyncQueue(__in ULONG const AllocationTag, __in ULONG const ITypeLinkOffset);

    BOOLEAN
    UnsafeTryAcquireActivity();

    VOID
    UnsafeReleaseActivity(ULONG ByCountOf = 1);

    //* Attempt to cancel a previously enqueued IType instance - _ThisLock is assumed to be held
    //
    //  Parameters:
    //      ItemToCancel - Reference to the IType to attempt to cancel
    //      ForPriority - Priority Queue that ItemToCancel is on
    //
    VOID __inline
    UnsafeCancelEnqueuedItem(ULONG ForPriority, IType& ItemToCancel);

    //* Enqueue an IType instance for async processing - _ThisLock is assumed to be held
    //
    //  Parameters:
    //      Priority                    - Indicates the priority of the enqueued IType. Valid values
    //                                    are 0-(NPriorities - 1); with 0 being the highest in that
    //                                    items at priority 0 will be dequeued before any items at
    //                                    lower level priorities.
    //      ItemToQueue                 - Item to enqueue
    //
    VOID __inline
    UnsafeEnqueue(IType& ItemToQueue, ULONG Priority = (NPriorities - 1));

protected:
    //* Derivation extension interface
    virtual VOID
    InvokeDroppedItemCallback(IType& DroppedItem);

    virtual IType*
    UnsafeGetNextEnqueuedItem(__in_opt ULONG *const PriorityOfDequeuedItem = nullptr);

    // This method is called under lock (_ThisLock) just before a DequeueOperation instance
    // is to completed with DequeuedItem as its completion results. The item has been dequeued
    // from _EnqueuedItemLists[PriorityOfDequeuedItem]. Typical use of overrides of this method
    // are for implementing starvation avoidance algorithms by a KAsyncQueue derivation. By
    // default this implementation does nothing.
    //
    // NOTE: It is assumed that a derivation overriding this method will not keep the thread
    //       for very long periods.
    virtual VOID
    OnUnsafeItemDequeued(IType& DequeuedItem, ULONG DequeuedItemPriority);

    // This method is called under lock (_ThisLock) just after an IType instance is enqueued.
    // The item has been enqueued at the tail of _EnqueuedItemLists[EnqueuedItemPriority].
    // Typical use of overrides of this method are for implementing starvation avoidance
    // algorithms by a KAsyncQueue derivation where this method can be used to do such things
    // as timestamping the EnqueuedItem. By default this implementation does nothing.
    //
    // NOTE: It is assumed that a derivation overriding this method will not keep the thread
    //       for very long periods.
    virtual VOID
    OnUnsafeItemEnqueued(IType& EnqueuedItem, ULONG EnqueuedItemPriority);

    virtual VOID
    OnUnsafeDequeuePaused();

    virtual VOID
    OnUnsafeDequeueResumed();

private:
    using KAsyncContextBase::Cancel;

    VOID
    OnStart();

    VOID
    OnCancel();

    VOID
    OnReuse();

    VOID
    UnsafePostWait(DequeueOperation& Waiter);

    VOID
    PostWait(DequeueOperation& Waiter);

    VOID
    CancelPostedWait(DequeueOperation& Waiter);

    DequeueOperation*
    UnsafeEnqueueOrGetWaiter(IType& ItemToQueue, ULONG Priority);

protected:
    class NodeList : public KNodeList<typename IType>
    {
    public:
        VOID
        SetOffset(ULONG LinkOffset)
        {
            this->_LinkOffset = LinkOffset;
        }
    };

    ULONG const                 _AllocationTag;
    DroppedItemCallback         _DroppedItemCallback;

    KSpinLock                   _ThisLock;          // Protects:
    ULONG                       _ThisVersion;       // Incremented on each Completion
    BOOLEAN                     _IsActive;

    // NOTE: Each item on these queues is represented by a count on _ActivityCount
    NodeList                    _EnqueuedItemLists[NPriorities];
    KNodeList<DequeueOperation> _Waiters;
private:
    ULONG                       _ActivityCount;
    BOOLEAN                     _IsDequeuePaused;
};


//*** KAsyncQueue Implementation
template<class IType, ULONG NPriorities>
KAsyncQueue<IType, NPriorities>::KAsyncQueue(__in ULONG const AllocationTag, __in ULONG const ITypeLinkOffset)
    :   _AllocationTag(AllocationTag),
        _IsActive(FALSE),
        _Waiters(FIELD_OFFSET(DequeueOperation, _Links)),
        _ActivityCount(0),
        _ThisVersion(1),
        _IsDequeuePaused(FALSE)
{
    for (ULONG ix = 0; ix < NPriorities; ix++)
    {
        _EnqueuedItemLists[ix].SetOffset(ITypeLinkOffset);
    }
}

template<class IType, ULONG NPriorities>
KAsyncQueue<IType, NPriorities>::~KAsyncQueue()
{
    KAssert(!_IsActive);
    KAssert(_Waiters.IsEmpty());
    for (ULONG ix = 0; ix < NPriorities; ix++)
    {
        KAssert(_EnqueuedItemLists[ix].IsEmpty());
    }
}

template<class IType, ULONG NPriorities>
NTSTATUS
KAsyncQueue<IType, NPriorities>::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __in ULONG const ITypeLinkOffset,
    __out KSharedPtr<KAsyncQueue<IType>>& Context)
{
    Context = _new(AllocationTag, Allocator) KAsyncQueue<IType, NPriorities>(AllocationTag, ITypeLinkOffset);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

template<class IType, ULONG NPriorities>
BOOLEAN
KAsyncQueue<IType, NPriorities>::UnsafeTryAcquireActivity()
//
// Try and acquire the right to use this instance. Once _ActivityCount reaches zero, no
// such right will be granted as the instance is in a deactivating state.
//
// Returns: FALSE - iif _ActivityCount has been latched at zero - only another Activate()
//                  will allow TRUE to be returned beyond that point.
//          TRUE  - _ActivityCount has been incremented safely and zero was not been touched
{
    if (!_IsActive || _ActivityCount == 0)
    {
        return FALSE;
    }

    _ActivityCount++;

    return TRUE;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::UnsafeReleaseActivity(ULONG ByCountOf)
//
// Release the right of usage acquired via UnsafeTryAcquireActivity().
//
// Completes operation cycle when _ActivityCount goes to zero.
//
{
    KAssert(_ActivityCount >= ByCountOf);
    _ActivityCount -= ByCountOf;

    if (_ActivityCount == 0)
    {
        BOOLEAN completing = Complete(STATUS_SUCCESS);
        KFatal(completing);

        _ThisVersion++;             // Invalidate the current version of this instance as viewed by
                                    // existing DequeueOperation instances produced by this Queue.
    }
}

template<class IType, ULONG NPriorities>
NTSTATUS
KAsyncQueue<IType, NPriorities>::Activate(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(!_IsActive);
        KAssert(_ActivityCount == 0);
        _IsActive = TRUE;
        _ActivityCount++;                       // #2
    }

    Start(ParentAsyncContext, CallbackPtr);
    return STATUS_PENDING;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnStart()
{
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::Deactivate(__in_opt DroppedItemCallback Callback)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(_IsActive);
        _IsActive = FALSE;              // Lock the API
    }

    _DroppedItemCallback = Callback;
    Cancel();
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::InvokeDroppedItemCallback(IType& DroppedItem)
{
    _DroppedItemCallback(*this, DroppedItem);
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnCancel()
{
    // trap Cancel calls not from Deactivate()
    KFatal(!_IsActive);

    // Wake up any suspended waiters with a status of K_STATUS_SHUTDOWN_PENDING
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        _ThisLock.Acquire();
             DequeueOperation* nextWaiter = _Waiters.RemoveHead();
        _ThisLock.Release();

        if (nextWaiter == nullptr)
        {
            break;
        }

        nextWaiter->CompleteDequeue(K_STATUS_SHUTDOWN_PENDING);
    }

    KNodeList<IType>    itemsToBeDropped(_EnqueuedItemLists[0].GetLinkOffset());
    IType*              item;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (_DroppedItemCallback)
        {
            for (ULONG ix = 0; ix < NPriorities; ix++)
            {
                // BUG, richhas, xxxxx, replace this with KNodeList<>::Move when available
                while ((item = _EnqueuedItemLists[ix].RemoveHead()) != nullptr)
                {
                    itemsToBeDropped.AppendTail(item);
                }
            }
        }

        UnsafeReleaseActivity();                            // Allow eventual Complete(); reverse #2 in Activate()
    }

    ULONG itemCountDropped = itemsToBeDropped.Count();

    if (itemCountDropped > 0)
    {
        while ((item = itemsToBeDropped.RemoveHead()) != nullptr)
        {
            InvokeDroppedItemCallback(*item);
        }

        K_LOCK_BLOCK(_ThisLock)
        {
             UnsafeReleaseActivity(itemCountDropped);       // allow eventual Complete();
                                                            // reverse #1 in UnsafeEnqueueOrGetWaiter() for
                                                            // any dropped items
        }
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnReuse()
{
    KAssert(!_IsActive);
    for (ULONG ix = 0; ix < NPriorities; ix++)
    {
        KAssert(_EnqueuedItemLists[ix].IsEmpty());
    }
    KAssert(_Waiters.IsEmpty());
    KAssert(_ActivityCount == 0);
}

template<class IType, ULONG NPriorities>
typename KAsyncQueue<IType, NPriorities>::DequeueOperation*
    KAsyncQueue<IType, NPriorities>::UnsafeEnqueueOrGetWaiter(IType& ItemToQueue, ULONG Priority)
{
    KAssert(Priority < NPriorities);
    KAssert(_ThisLock.IsOwned() && _IsActive);

    DequeueOperation* nextWaiter = _IsDequeuePaused ? nullptr : _Waiters.RemoveHead();

    if (nextWaiter == nullptr)
    {
        // No pending waiter - just queue the item - represented as an activity count
        BOOLEAN acquired = UnsafeTryAcquireActivity();  // #1
        KAssert(acquired);
#if !DBG
        UNREFERENCED_PARAMETER(acquired);
#endif

        _EnqueuedItemLists[Priority].AppendTail(&ItemToQueue);
        OnUnsafeItemEnqueued(ItemToQueue, Priority);
    }

    return nextWaiter;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::UnsafeEnqueue(IType& ItemToQueue, ULONG Priority)
{
    KFatal(Priority < NPriorities);
    KFatal(_ThisLock.IsOwned() && _IsActive);
    DequeueOperation* nextWaiter = UnsafeEnqueueOrGetWaiter(ItemToQueue, Priority);

    if (nextWaiter != nullptr)
    {
        // Have waiter - dequeue dispatch will occur below - inform the derivation
        OnUnsafeItemDequeued(ItemToQueue, Priority);

        // just dispatch the enqueued item directly to waiter (UnsafeEnqueueOrGetWaiter)
        nextWaiter->CompleteDequeue(ItemToQueue);
    }
}

template<class IType, ULONG NPriorities>
NTSTATUS
KAsyncQueue<IType, NPriorities>::Enqueue(IType& ItemToQueue, ULONG Priority)
{
    KFatal(Priority < NPriorities);
    DequeueOperation* nextWaiter = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsActive)
        {
            return K_STATUS_SHUTDOWN_PENDING;
        }

        nextWaiter = UnsafeEnqueueOrGetWaiter(ItemToQueue, Priority);
        if (nextWaiter != nullptr)
        {
            // Have waiter - dequeue dispatch will occur below - inform the derivation
            OnUnsafeItemDequeued(ItemToQueue, Priority);
        }
    }

    if (nextWaiter != nullptr)
    {
        // just dispatch the enqueued item directly to waiter - just removed from _Waiters above
        // from UnsafeEnqueueOrGetWaiter
        nextWaiter->CompleteDequeue(ItemToQueue);
    }

    return STATUS_SUCCESS;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::UnsafeCancelEnqueuedItem(ULONG ForPriority, IType& ItemToCancel)
{
    KAssert(_ThisLock.IsOwned() && _IsActive);
    KFatal(ForPriority < NPriorities);
    IType* removedItem = _EnqueuedItemLists[ForPriority].Remove(&ItemToCancel);

    BOOLEAN wasRemoved = (removedItem != nullptr);
    KFatal(wasRemoved);

    // release corresponding activity count #1 - see UnsafeEnqueueOrGetWaiter()
    UnsafeReleaseActivity();
}

template<class IType, ULONG NPriorities>
BOOLEAN
KAsyncQueue<IType, NPriorities>::CancelEnqueue(IType& ItemToCancel)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsActive)
        {
            return FALSE;
        }

        for (ULONG ix = 0; ix < NPriorities; ix++)
        {
            if (_EnqueuedItemLists[ix].IsOnList(&ItemToCancel))
            {
                UnsafeCancelEnqueuedItem(ix, ItemToCancel);
                return TRUE;
            }
        }
    }

    return FALSE;
}

template<class IType, ULONG NPriorities>
NTSTATUS
KAsyncQueue<IType, NPriorities>::CancelAllEnqueued(__in DroppedItemCallback Callback)
{
    if (TryAcquireActivities())
    {
        // Queue instance is held form completing and is an active instance
        KFinally([this](){ ReleaseActivities(); });		

        KNodeList<IType>    itemsToBeDropped(_EnqueuedItemLists[0].GetLinkOffset());
        IType*              item;

        K_LOCK_BLOCK(_ThisLock)
        {
            for (ULONG ix = 0; ix < NPriorities; ix++)
            {
                // BUG, richhas, xxxxx, replace this with KNodeList<>::Move when available
                while ((item = _EnqueuedItemLists[ix].RemoveHead()) != nullptr)
                {
                    itemsToBeDropped.AppendTail(item);
                }
            }
        }

        ULONG itemCountDropped = itemsToBeDropped.Count();

        if (itemCountDropped > 0)
        {
            while ((item = itemsToBeDropped.RemoveHead()) != nullptr)
            {
                Callback(*this, *item);
            }

            K_LOCK_BLOCK(_ThisLock)
            {
                 UnsafeReleaseActivity(itemCountDropped);       // allow eventual Complete();
                                                                // reverse #1 in UnsafeEnqueueOrGetWaiter() for
                                                                // any dropped items
            }
        }

        return STATUS_SUCCESS;
    }

    return K_STATUS_NOT_STARTED;
}

template<class IType, ULONG NPriorities>
IType*
KAsyncQueue<IType, NPriorities>::UnsafeGetNextEnqueuedItem(__in_opt ULONG *const PriorityOfDequeuedItem)
{
    for (ULONG ix = 0; ix < NPriorities; ix++)
    {
        if (!_EnqueuedItemLists[ix].IsEmpty())
        {
            if (PriorityOfDequeuedItem != nullptr)
            {
                *PriorityOfDequeuedItem = ix;
            }
            return _EnqueuedItemLists[ix].RemoveHead();
        }
    }

    return nullptr;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::UnsafePostWait(DequeueOperation& Waiter)
{
    IType*  queuedItem = nullptr;
    ULONG   queuedItemPriority = 0;
    BOOLEAN indicateDoingShutdown = FALSE;

    if (Waiter._OwnerVersion != _ThisVersion)
    {
        KAssert(Waiter._OwnerVersion < _ThisVersion);

        // The calling DequeueOperation is for an older version of this Queue - tell caller it's being/been shutdown
        indicateDoingShutdown = TRUE;
    }
    else
    {
        queuedItem = _IsDequeuePaused ? nullptr : UnsafeGetNextEnqueuedItem(&queuedItemPriority);
        if (queuedItem == nullptr)
        {
            // No items enqueued
            indicateDoingShutdown = !_IsActive;
            if (_IsActive)
            {
                // Still active just suspend Waiter
                _Waiters.AppendTail(&Waiter);
            }
        }
        else
        {
            // did pull queued item
            OnUnsafeItemDequeued(*queuedItem, queuedItemPriority);

            // release corresponding activity count #1 - see UnsafeEnqueueOrGetWaiter()
            UnsafeReleaseActivity();
        }
    }

    if (queuedItem != nullptr)
    {
        // have item to process - complete Waiter with it
        Waiter.CompleteDequeue(*queuedItem);
    }
    else if (indicateDoingShutdown)
    {
        // Queue is drained and we are shutting down
        Waiter.CompleteDequeue(K_STATUS_SHUTDOWN_PENDING);
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::PostWait(DequeueOperation& Waiter)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        UnsafePostWait(Waiter);
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::CancelPostedWait(DequeueOperation& Waiter)
{
    BOOLEAN doComplete = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        doComplete = (_Waiters.Remove(&Waiter) != nullptr);
    }

    if (doComplete)
    {
        Waiter.CompleteDequeue(STATUS_CANCELLED);
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnUnsafeItemDequeued(
    IType&,
    ULONG)
{
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnUnsafeItemEnqueued(
    IType&,
    ULONG)
{
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::PauseDequeue(
    )
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsDequeuePaused)
        {
            _IsDequeuePaused = TRUE;

            OnUnsafeDequeuePaused();
        }
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::ResumeDequeue(
    )
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (_IsDequeuePaused)
        {
            _IsDequeuePaused = FALSE;

            while ((GetNumberOfWaiters() > 0) && (GetNumberOfQueuedItems() > 0))
            {
                DequeueOperation* nextWaiter = _Waiters.RemoveHead();
                UnsafePostWait(*nextWaiter);
            }

            OnUnsafeDequeueResumed();
        }
    }
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnUnsafeDequeuePaused()
{
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::OnUnsafeDequeueResumed()
{
}

//* Test support methods
template<class IType, ULONG NPriorities>
ULONG
KAsyncQueue<IType, NPriorities>::GetNumberOfQueuedItems()
{
    ULONG result = 0;

    for (ULONG ix = 0; ix < NPriorities; ix++)
    {
        result += _EnqueuedItemLists[ix].Count();
    }
    return result;
}

template<class IType, ULONG NPriorities>
ULONG
KAsyncQueue<IType, NPriorities>::GetNumberOfWaiters()
{
    return _Waiters.Count();
}


//** DequeueOperation imp
template<class IType, ULONG NPriorities>
NTSTATUS
KAsyncQueue<IType, NPriorities>::CreateDequeueOperation(__out KSharedPtr<DequeueOperation>& Context)
{
    NTSTATUS status = STATUS_SUCCESS;

    Context = _new(_AllocationTag, GetThisAllocator()) DequeueOperation(*this, _ThisVersion);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }

    return status;
}

template<class IType, ULONG NPriorities>
KAsyncQueue<IType, NPriorities>::DequeueOperation::DequeueOperation(KAsyncQueue<IType, NPriorities>& Owner, ULONG const OwnerVersion)
    :   _Owner(&Owner),
        _OwnerVersion(OwnerVersion),
        _Result(nullptr)
{
}

template<class IType, ULONG NPriorities>
KAsyncQueue<IType, NPriorities>::DequeueOperation::~DequeueOperation()
{
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::StartDequeue(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    Start(ParentAsyncContext, CallbackPtr);
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::OnStart()
{
    _Owner->PostWait(*this);
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::CompleteDequeue(IType& ItemDequeued)
{
    _Result = &ItemDequeued;

    BOOLEAN didComplete = Complete(STATUS_SUCCESS);
    KAssert(didComplete);
#if !DBG
    UNREFERENCED_PARAMETER(didComplete);
#endif        
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::CompleteDequeue(NTSTATUS Status)
{
    BOOLEAN didComplete = Complete(Status);
    KAssert(didComplete);
#if !DBG
    UNREFERENCED_PARAMETER(didComplete);
#endif
}

template<class IType, ULONG NPriorities>
IType&
KAsyncQueue<IType, NPriorities>::DequeueOperation::GetDequeuedItem()
{
    KFatal(_Result != nullptr);
    return *_Result;
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::OnCancel()
{
    _Owner->CancelPostedWait(*this);
}

template<class IType, ULONG NPriorities>
VOID
KAsyncQueue<IType, NPriorities>::DequeueOperation::OnReuse()
{
    _Result = nullptr;
}



//** KAsyncOrderedGate class
//
// Description:
//      This template is used to generate a strongly typed ordered gate (queue) class for items of type IType
//      and with an order key of type OType. It is a Service in its own right and as such follows the Activate()
//      and Deactivate() pattern. This implementation builds off of KAsyncQueue's but hides part of that public
//      interface and augments with a bit of different flavor such that items of IType will only be dequeued in
//      a defined order. The initial value (the first expected) for this order (aka Order Slot) is supplied at
//      Activation time. Only a enqueued item with the next expected order value will be allowed to be dequeued;
//      all others (greater than) this next expected order value will stay queued reguardless of available
//      number of OrderedDequeueOperations that are outstanding. To support this notion of order values, IType's
//      implementation must supply two methods that abstract these values. One is GetAsyncOrderedGateKey() which
//      is used to obtain a OType& for the order value of a given instance of IType. GetNextAsyncOrderedGateKey()
//      is used by KAsyncOrderedGate to obtain the next logical order value from and IType instance. Specifically
//      the following MUST be implemented by an KAsyncOrderedGate IType:
//
//           OType IType::GetAsyncOrderedGateKey()
//           OType IType::GetNextAsyncOrderedGateKey()
//
//      The other requirement is that the implementation of OType must include the standard set of comparison
//      operators.
//
//      In all aspects this class behaves the same as KAsyncQueue on the consumption (dequeue side).
//
template <class IType, typename OType>
class KAsyncOrderedGate sealed : public KAsyncQueue<IType>
{
    K_FORCE_SHARED(KAsyncOrderedGate);

private:
    // Hide KAsyncQueue's public interface
    using KAsyncQueue<IType>::Create;
    using typename KAsyncQueue<IType>::DroppedItemCallback;
    using KAsyncQueue<IType>::Activate;
    using KAsyncQueue<IType>::Deactivate;
    using KAsyncQueue<IType>::Enqueue;
    using KAsyncQueue<IType>::CancelEnqueue;
    using typename KAsyncQueue<IType>::DequeueOperation;
    using KAsyncQueue<IType>::CreateDequeueOperation;

public:
    //* Factory for creation of KAsyncOrderedGate Service instances.
    //
    //  Parameters:
    //      AllocationTag, Allocator        - Standard memory allocation component and tag
    //      ITypeLinkOffset                 - FIELD_OFFSET(IType, Links) - where Links is a KListEntry
    //                                        data member in IType to be used for queuing instances
    //      Context                         - SPtr that points to an allocated KAsyncOrderedGate<IType> instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated KAsyncOrderedGate<IType> instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in ULONG const ITypeLinkOffset,
        __out KSharedPtr<KAsyncOrderedGate<IType, OType>>& Context);

    //* Delegate for processing dropped IType instances during deactivation
    //
    typedef KDelegate<VOID(
        KAsyncOrderedGate<IType, OType>&    DeactivatingGate,
        IType&                              DroppedItem
    )> DroppedGateItemCallback;


    //* Activate an instance of a KAsyncOrderedGate<> Service.
    //
    // KAsyncOrderedGate Service instances that are useful are an KAsyncContext in the operational state.
    // This means that the supplied callback (Callback) will be invoked once DeactivateGate() is called
    // and the instance has finished its shutdown process - meaning any queued item have been dispatched
    // for processing by a OrderedDequeueOperation instance - meaning the completion process started on the
    // corresponding DequeueOperation. NOTE: This means that the final successful OrderedDequeueOperation
    // completion can race with the completion of the owning KAsyncOrderedGate triggered by DeactivateGate().
    //
    // Returns: STATUS_PENDING - Service successful started. Once this status is returned, that
    //                           instance will accept other operations until Deactive() is called.
    //
    NTSTATUS
    ActivateGate(
        __in OType InitialOrderedValue,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback Callback);

    //* Trigger shutdown on an active KAsyncOrderedGate<> Service instance.
    //
    // Once called, a Deactivated (or deactivating) KAsyncOrderedGate will reject any further operations with
    // a Status or result of K_STATUS_SHUTDOWN_PENDING. Any existing queued items will be drained via
    // OrderedDequeueOperation completions unless an optional DroppedGateItemCallback is provided. If such a
    // callback is provided, that callback will be called for any and all IType items that are still queued at that
    // time BEFORE completion occurs on the KAsyncOrderedGate.
    //
    // Parameters:
    //      Callback - optional callback that is invoked for each dropped IType item.
    //
    VOID
    DeactivateGate(__in_opt DroppedGateItemCallback Callback = nullptr);

    //* Enqueue an IType instance for async processing in the order defined by the OType returned
    //  by the method, IType::GetAsyncOrderedGateKey(), required to be implemented on type IType. Once
    //  the enqueued item is dequeued for processing, it establishes the next expected order value via
    //  the method IType::GetNextAsyncOrderedGateKey().
    //
    // Returns:
    //      STATUS_SUCCESS                  - Item is queued
    //      K_STATUS_SHUTDOWN_PENDING       - Queue is in the process of shutting down
    //      STATUS_OBJECT_NAME_COLLISION    - The item's order value (GetAsyncOrderedGateKey()) is not
    //                                        unique.
    //      STATUS_OBJECT_NAME_INVALID      - The OType order value of ItemToQueue is lower than the next
    //                                        expected order slot for the Gate.
    //
    NTSTATUS
    EnqueueOrdered(IType& ItemToQueue);

    //* Override the current next-value of the gate.
    //
    //  NOTE: This method may only be called when there are no enqueued items.
    //
    VOID
    SetNextOrderedItemValue(OType Value);

    //* KAsyncOrderedGate specific DequeueOperation
    class OrderedDequeueOperation : public KAsyncQueue<IType>::DequeueOperation
    {
        K_FORCE_SHARED(OrderedDequeueOperation);

    private:
        friend class KAsyncOrderedGate;
        OrderedDequeueOperation(KAsyncOrderedGate<IType, OType>& Owner, ULONG const OwnerVersion);
    };

    //* OrderedDequeueOperation Factory
    //
    //  Parameters:
    //      Context                         - SPtr that points to the allocated OrderedDequeueOperation instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated OrderedDequeueOperation instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    //  NOTE: Instances of OrderedDequeueOperation created by this factory are bound to the producing KAsyncOrderedGate
    //        instance AND the specific Activation-version of that KAsyncOrderedGate and may only be used with
    //        that binding.
    //
    NTSTATUS
    CreateOrderedDequeueOperation(__out KSharedPtr<OrderedDequeueOperation>& Context);

private:
    KAsyncOrderedGate(__in ULONG const AllocationTag, __in ULONG const ITypeLinkOffset);

    VOID
    UnsafeSetNextOrderedItemValue(IType& FromItem);

    VOID
    InvokeDroppedItemCallback(IType& DroppedItem);      // Called by KAsyncQueue

    IType*
    UnsafeGetNextEnqueuedItem(__in_opt ULONG *const PriorityOfDequeuedItem);  // Called by KAsyncQueue

private:
    OType               _NextOrderedItemValue;          // Protected by _ThisLock in KAsyncQueue
};


//** KAsyncOrderedGate<> Implementation
template<class IType, typename OType>
KAsyncOrderedGate<IType, OType>::KAsyncOrderedGate(__in ULONG const AllocationTag, __in ULONG const ITypeLinkOffset)
    :   KAsyncQueue<IType>(AllocationTag, ITypeLinkOffset)
{
}

template<class IType, typename OType>
KAsyncOrderedGate<IType, OType>::~KAsyncOrderedGate()
{
}

template<class IType, typename OType>
NTSTATUS
KAsyncOrderedGate<IType, OType>::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __in ULONG const ITypeLinkOffset,
    __out KSharedPtr<KAsyncOrderedGate<IType, OType>>& Context)
{
    Context = _new(AllocationTag, Allocator) KAsyncOrderedGate<IType, OType>(AllocationTag, ITypeLinkOffset);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

template<class IType, typename OType>
VOID
KAsyncOrderedGate<IType, OType>::UnsafeSetNextOrderedItemValue(IType& FromItem)
{
    OType nextOrderValue = FromItem.GetNextAsyncOrderedGateKey();
    KFatal(nextOrderValue > _NextOrderedItemValue);
    _NextOrderedItemValue = nextOrderValue;
}

template<class IType, typename OType>
NTSTATUS
KAsyncOrderedGate<IType, OType>::ActivateGate(
    __in OType InitialOrderedValue,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _NextOrderedItemValue = InitialOrderedValue;
    return Activate(ParentAsyncContext, CallbackPtr);
}

template<class IType, typename OType>
VOID
KAsyncOrderedGate<IType, OType>::DeactivateGate(__in_opt DroppedGateItemCallback Callback)
{
    // BUG, richhas, xxxxx, consider other apporaches for stronger type equiv test
    static_assert(
        sizeof(DroppedGateItemCallback) == sizeof(typename KAsyncQueue<IType>::DroppedItemCallback),
        "KAsyncOrderedGate::DroppedGateItemCallback type mis-match with KAsyncQueue::DroppedItemCallback");

    Deactivate(*((typename KAsyncQueue<IType>::DroppedItemCallback*)((void*)&Callback)));
}

template<class IType, typename OType>
NTSTATUS
KAsyncOrderedGate<IType, OType>::EnqueueOrdered(IType& ItemToQueue)
{
    OrderedDequeueOperation*    nextWaiter = nullptr;
    OType                       insertKey = ItemToQueue.GetAsyncOrderedGateKey();

    K_LOCK_BLOCK(this->_ThisLock)
    {
        if (!this->_IsActive)
        {
            return K_STATUS_SHUTDOWN_PENDING;
        }

        if (insertKey < _NextOrderedItemValue)
        {
            // Have progressed further in the order then ItemToQueue's order slot
            return STATUS_OBJECT_NAME_INVALID;
        }

        if (insertKey == _NextOrderedItemValue)
        {
            // Item being enqueued is the next expected item in the order - attempt to schedule it
            nextWaiter = (OrderedDequeueOperation*)this->_Waiters.RemoveHead();
            if (nextWaiter != nullptr)
            {
                UnsafeSetNextOrderedItemValue(ItemToQueue);
            }
        }

        if (nextWaiter == nullptr)
        {
            // no pending waiter or not the next in expected order - insert it in _EnqueuedItemLists in order
            IType* listItem = this->_EnqueuedItemLists[0].PeekTail();
            OType  listItemKey;
            OType& listItemKeyRef = listItemKey;  // Access this to avoid warning C4701

            while ((listItem != nullptr) && ((listItemKeyRef = listItem->GetAsyncOrderedGateKey()) > insertKey))
            {
                listItem = this->_EnqueuedItemLists[0].Predecessor(listItem);
            }

            if (listItem != nullptr)
            {
                if (insertKey == listItemKeyRef)
                {
                    // No dups allowed
                    return STATUS_OBJECT_NAME_COLLISION;
                }
                KAssert(insertKey > listItemKeyRef);

                this->_EnqueuedItemLists[0].InsertAfter(&ItemToQueue, listItem);
            }
            else
            {
                // just put at top of list
                this->_EnqueuedItemLists[0].InsertHead(&ItemToQueue);
            }

            BOOLEAN acquired = this->UnsafeTryAcquireActivity();  // see KAsynQueue #1
            KAssert(acquired);

            //
            // Make /W4 happy - error C4189: 'acquired' : local variable is initialized but not referenced
            //
            acquired;
        }
    }

    if (nextWaiter != nullptr)
    {
        // just dispatch the enqueued item directly to waiter - just removed from _Waiters above
        nextWaiter->CompleteDequeue(ItemToQueue);

        // While there are available waiters AND queued items of the expected order slots - dispatch...
        do
        {
            nextWaiter = nullptr;
            IType* itemToProcess = nullptr;

            K_LOCK_BLOCK(this->_ThisLock)
            {
                itemToProcess = this->_EnqueuedItemLists[0].PeekHead();
                if ((itemToProcess != nullptr) && (itemToProcess->GetAsyncOrderedGateKey() == _NextOrderedItemValue))
                {
                    // Next enqueued item is the next expected item in the order - attempt to schedule it
                    nextWaiter = (OrderedDequeueOperation*)this->_Waiters.RemoveHead();
                    if (nextWaiter != nullptr)
                    {
                        itemToProcess = this->_EnqueuedItemLists[0].RemoveHead();
                        UnsafeSetNextOrderedItemValue(*itemToProcess);
                        this->UnsafeReleaseActivity();
                    }
                }
            }

            if (nextWaiter != nullptr)
            {
                KAssert(itemToProcess != nullptr);
                nextWaiter->CompleteDequeue(*itemToProcess);
            }
        } while (nextWaiter != nullptr);
    }

    return STATUS_SUCCESS;
}

template<class IType, typename OType>
VOID
KAsyncOrderedGate<IType, OType>::SetNextOrderedItemValue(OType Value)
{
    K_LOCK_BLOCK(this->_ThisLock)
    {
        KFatal(this->_EnqueuedItemLists[0].Count() == 0);
        _NextOrderedItemValue = Value;
    }
}

template<class IType, typename OType>
VOID
KAsyncOrderedGate<IType, OType>::InvokeDroppedItemCallback(IType& DroppedItem)
// Called by KAsyncQueue::OnCancel() (Deactivation)
{
    this->_DroppedItemCallback(*this, DroppedItem);
}

template<class IType, typename OType>
IType*
KAsyncOrderedGate<IType, OType>::UnsafeGetNextEnqueuedItem(__in_opt ULONG *const PriorityOfDequeuedItem)
// Called by KAsyncQueue::PostWait()
{
    if (PriorityOfDequeuedItem != nullptr)
    {
        *PriorityOfDequeuedItem = 0;
    }

    IType* head = this->_EnqueuedItemLists[0].PeekHead();
    if (head != nullptr)
    {
        if (head->GetAsyncOrderedGateKey() == _NextOrderedItemValue)
        {
            // top item is the next expected in the order; update the next expected in that order
            IType* item = this->_EnqueuedItemLists[0].RemoveHead();
            KAssert(item == head);

            UnsafeSetNextOrderedItemValue(*item);
         }
        else
        {
            head = nullptr;
        }
    }

    return head;
}


//** OrderedDequeueOperation imp
template<class IType, typename OType>
NTSTATUS
KAsyncOrderedGate<IType, OType>::CreateOrderedDequeueOperation(__out KSharedPtr<OrderedDequeueOperation>& Context)
{
    NTSTATUS status = STATUS_SUCCESS;

    Context = _new(this->_AllocationTag, this->GetThisAllocator()) OrderedDequeueOperation(*this, this->_ThisVersion);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }

    return status;
}

template<class IType, typename OType>
KAsyncOrderedGate<IType, OType>::OrderedDequeueOperation::OrderedDequeueOperation(
    KAsyncOrderedGate<IType, OType>& Owner,
    ULONG const OwnerVersion)
    :   KAsyncQueue<IType>::DequeueOperation(*(static_cast<KAsyncQueue<IType>*>(&Owner)), OwnerVersion)
{
}

template<class IType, typename OType>
KAsyncOrderedGate<IType, OType>::OrderedDequeueOperation::~OrderedDequeueOperation()
{
}
