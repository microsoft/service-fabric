/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KAsyncContext.cpp

    Description:
      Kernel Template Library (KTL): Async Continuation and Lock Support Implementation

    History:
      richhas          23-Sep-2010         Initial version.

--*/

#include "ktl.h"
#include "KLinkList.h"
#include "KAsyncLock.h"
#include "KAsyncContext.h"

//*** KAsyncContextBase Implementation
//
// Non-inline implementation
BOOLEAN
KAsyncContextBase::UnsafeCancelPendingAcquireLock() // TRUE if canceled
{
    // KAssert(_ThisLock.IsHeld());
    return ((_LockPtr != nullptr) && _LockPtr->CancelAcquire(this));
}

NOFAIL
KAsyncContextBase::KAsyncContextBase()
{
    _CurrentStateDispatcher = &KAsyncContextBase::InitializedStateDispatcher;
    KObject<KAsyncContextBase>::SetStatus(K_STATUS_NOT_STARTED);
    _PendingWorkCount = 0;
    _LocksHeld = 0;
    _LockPtr = nullptr;
    _ParentAsyncContextPtr = nullptr;
    InitializeWaitListEntry();
    this->_CurrentDispatchThreadId = KThread::InvalidThreadId;
    #if KTL_USER_MODE
    {
        _ApartmentState = nullptr;
        _ExecutionState._KtlUsingSystemThreadPool = GetThisKtlSystem().GetDefaultSystemThreadPoolUsage();
        _ExecutionState._UsingSystemThreadPool = _ExecutionState._KtlUsingSystemThreadPool;
        _ExecutionState._DetachThreadOnCompletion = FALSE;
    }
    #else
    {
        _ExecutionState._IsDPCDispatch = FALSE;
        _ExecutionState._IsCurrentlyDPCDispatch = FALSE;
        _KDPC = NULL;
    }
    #endif
}

KAsyncContextBase::~KAsyncContextBase()
{
    KInvariant(IsReusable());
    #if KTL_USER_MODE
    {
        KInvariant(_ApartmentState == nullptr);
    }

    #endif
}


//* Public API
NTSTATUS
KAsyncContextBase::Status()
{
    // Only expose real status when the instance is actually in a reuseable state - not active
    NTSTATUS result;

    _ThisLock.Acquire();
    {
        if (IsReusable())
        {
            result = KObject<KAsyncContextBase>::Status();
        }
        else
        {
            result = STATUS_PENDING;
        }
    }
    _ThisLock.Release();

    return result;
}

BOOLEAN
KAsyncContextBase::Cancel()
{
    return ScheduleCancel();
}

VOID
KAsyncContextBase::Reuse()
{
    _ThisLock.Acquire();
    {
        KInvariant(IsReusable());
        KInvariant(!IsAcquiringLock());
        KInvariant(!WaitListEntry().IsLinked());
        KInvariant(_ExecutionState.IsClear());
        KInvariant(_CompletionQueue.IsEmpty());
        KInvariant(_PendingWorkCount == 0);
        KInvariant(_ParentAsyncContextPtr == nullptr);

        // ** ChangeState
        _CurrentStateDispatcher = &KAsyncContextBase::InitializedStateDispatcher;
        KObject<KAsyncContextBase>::SetStatus(K_STATUS_NOT_STARTED);
        #if KTL_USER_MODE
        {
            KInvariant(_ApartmentState == nullptr);
            _ExecutionState._UsingSystemThreadPool = _ExecutionState._KtlUsingSystemThreadPool;
            _ExecutionState._DetachThreadOnCompletion = FALSE;
        }
        #endif
    }
    _ThisLock.Release();

    _CurrentDispatchThreadId = KThread::GetCurrentThreadId();
    OnReuse();
    _CurrentDispatchThreadId = KThread::InvalidThreadId;
}

BOOLEAN
KAsyncContextBase::IsInApartment()
{
    return KThread::GetCurrentThreadId() == _CurrentDispatchThreadId;
}

#if KTL_USER_MODE
VOID
KAsyncContextBase::SetDefaultSystemThreadPoolUsage(BOOLEAN OnOrOff)
{
    _ThisLock.Acquire();
    {
        KInvariant(IsReusable());
        _ExecutionState._UsingSystemThreadPool = OnOrOff;
    }
    _ThisLock.Release();
}

BOOLEAN
KAsyncContextBase::GetDefaultSystemThreadPoolUsage()
{
    return _ExecutionState._UsingSystemThreadPool;
}

VOID
KAsyncContextBase::SetDetachThreadOnCompletion(BOOLEAN OnOrOff)
{
    _ThisLock.Acquire();
    {
        KInvariant(IsInitializedState() || IsStarting() || IsOperatingState());
        _ExecutionState._DetachThreadOnCompletion = OnOrOff;
    }
    _ThisLock.Release();
}

BOOLEAN
KAsyncContextBase::GetDetachThreadOnCompletion()
{
    return _ExecutionState._DetachThreadOnCompletion;
}
#else
NTSTATUS
KAsyncContextBase::SetDispatchOnDPCThread(
    __in BOOLEAN SetDPCDispatch
    )
{
    _ThisLock.Acquire();
    {
        if ((SetDPCDispatch) && (! _KDPC))
        {
            _KDPC = _new(GetThisAllocationTag(), GetThisAllocator()) KDPC;

            if (_KDPC)
            {
                KeInitializeDpc(_KDPC, KAsyncContextBase::DPCHandler, this);
            } else {
                _ThisLock.Release();
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

        _ExecutionState._IsDPCDispatch = SetDPCDispatch;
        _ExecutionState._IsCurrentlyDPCDispatch = SetDPCDispatch;
    }
    _ThisLock.Release();

    return(STATUS_SUCCESS);
}
#endif


//* Derivation support API
VOID
KAsyncContextBase::Start(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CompletionCallback CallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    KInvariant(KObject<KAsyncContextBase>::Status() == K_STATUS_NOT_STARTED);
    KAsyncGlobalContext* globalContext = GlobalContextOverride;

    AddRef();       // #1: As long as this instance is busy (not completed), keep it alive
    if (ParentAsyncContext != nullptr)
    {
        // Inform owner that a new sub op has started - down counted when Complete is called (indirectly)
        ParentAsyncContext->SubOpStarted();
        if (globalContext == nullptr)
        {
            // No Global context override supplied by caller - use the parent's global context
            globalContext = ParentAsyncContext->_GlobalContext.RawPtr();
        }
    }
    ScheduleStart(ParentAsyncContext, CallbackPtr, globalContext);
}

BOOLEAN
KAsyncContextBase::Complete(__in NTSTATUS Status)
{
    return ScheduleComplete(Status, nullptr, nullptr);
}

BOOLEAN
KAsyncContextBase::Complete(
    __in_opt CompletingCallback Callback,
    __in_opt void* CallbackContext,
    __in NTSTATUS Status)
{
    return ScheduleComplete(Status, Callback, CallbackContext);
}

VOID
KAsyncContextBase::AcquireLock(__in KAsyncLock& LockRef, __in LockAcquiredCallback AcquiredCallback)
{
    BOOLEAN doScheduleComplete;

    _ThisLock.Acquire();
    {
        KInvariant(IsOperatingState() || IsCompletingState());
        KInvariant(!IsAcquiringLock());
        KInvariant(!WaitListEntry().IsLinked());                    // WaitList entry must be used to queue
                                                                // this kasync on LockRef
        _LockPtr = &LockRef;
        _AcquiredLockCallback = AcquiredCallback;

        KInvariant(!_ExecutionState._AcquireLockRequested);
        _PendingWorkCount++;
        if ((doScheduleComplete = _LockPtr->AcquireLock(this)) == FALSE)
        {
            _ExecutionState._AcquireLockRequested = true;       // Record we have posted an acquire lock
        }
    }
    _ThisLock.Release();

    if (doScheduleComplete)
    {
        // Got the lock during Acquire - must schedule our own completion.
        ScheduleLockAcquireCompletion();
        // BUG, richhas, xxxxxx, Consider adding an optional short circuit where AcquiredCallback is
        //                       dispatched directly from here when sync acquired happens.
    }
}

VOID
KAsyncContextBase::ReleaseLock(__in KAsyncLock& LockRef)
{
    LockRef.ReleaseLock(this);
    LONG result = InterlockedDecrement(&_LocksHeld);
    KInvariant(result >= 0);        // NOTE: this test assumes that the corresponding inc (#1) in
                                //       ScheduleLockAcquireCompletion() is done before
                                //       actually scheduling the lock completion in the SM.
}

BOOLEAN
KAsyncContextBase::IsLockOwner(__in KAsyncLock& LockRef)
{
    return LockRef.IsLockOwner(this);
}

BOOLEAN
KAsyncContextBase::TryAcquireActivities(ULONG NumberToAcquire)
{
    BOOLEAN activityAcquired = FALSE;

    _ThisLock.Acquire();
    {
        if (IsOperatingState() || IsCompletingState() || IsStarting())
        {
            _PendingWorkCount += NumberToAcquire;
            activityAcquired = TRUE;

        }
    }
    _ThisLock.Release();

    return activityAcquired;
}

VOID
KAsyncContextBase::AcquireActivities(ULONG NumberToAcquire)
{
    BOOLEAN activityAcquired = TryAcquireActivities(NumberToAcquire);
    KInvariant(activityAcquired);
}

BOOLEAN
KAsyncContextBase::ReleaseActivities(ULONG NumberToRelease)
{
    BOOLEAN scheduleThread = FALSE;
    BOOLEAN result = FALSE;

    _ThisLock.Acquire();
    {
        KInvariant(IsOperatingState() || IsCompletingState() || IsStarting());
        KInvariant(_PendingWorkCount >= NumberToRelease);
        _PendingWorkCount -= NumberToRelease;
        result = (_PendingWorkCount == 0);

        // iif the sm (only in CompletingState) could be waiting for _PendingWorkCount -> 0;
        if (result && IsCompletingState())
        {
            // NOTE: Results (at least) in a NoEvent return from UnsafeGetNextEvent() in the
            //       CompletingStateDispatcher
            scheduleThread = UnsafeEnsureThread();
        }
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }

    return result;
}

ULONG
KAsyncContextBase::QueryActivityCount()
{
    return _PendingWorkCount;
}

KAsyncGlobalContext::SPtr&
KAsyncContextBase::GetGlobalContext()
{
    return _GlobalContext;
}

VOID
KAsyncContextBase::SetGlobalContext(KAsyncGlobalContext::SPtr GlobalContext)
{
    KInvariant( !_GlobalContext );
    _GlobalContext = Ktl::Move(GlobalContext);
}

KActivityId
KAsyncContextBase::GetActivityId()
{
    return KAsyncGlobalContext::GetActivityId(_GlobalContext);
}

// ReleaseApartment()  -- See in the dispatcher section


//* Derivation extension API: default implementation
VOID
KAsyncContextBase::OnStart()
{
    // Default to completeting as success
    Complete(STATUS_SUCCESS);
}

VOID
KAsyncContextBase::OnCancel()
{
}

VOID
KAsyncContextBase::OnReuse()
{
}

VOID
KAsyncContextBase::OnCompleted()
{
}

//* Private implementation
KAsyncContextBase::EventFlags::EventFlags()
{
    _StartPending = FALSE;
    _CompletePending = FALSE;
    _CancelPending = FALSE;
    _WaitCompletePending = FALSE;
    _AlertCompletePending = FALSE;
    _CompletionDonePending = FALSE;
}

KAsyncContextBase::ExecutionState::ExecutionState()
{
    _DispatcherState = Idle;
    _AcquireLockRequested = FALSE;
    #if KTL_USER_MODE
    {
        _UsingSystemThreadPool = FALSE;
        _KtlUsingSystemThreadPool = FALSE;
        _DetachThreadOnCompletion = FALSE;
    }
    #else
    {
        _IsDPCDispatch = FALSE;
        _IsCurrentlyDPCDispatch = FALSE;
    }
    #endif
}

// scheduling primitives
VOID
KAsyncContextBase::ScheduleStart(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in CompletionCallback CallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContext)
{
    BOOLEAN scheduleThread = FALSE;

    _ThisLock.Acquire();

    {
        KInvariant(IsInitializedState());
        KInvariant(!_ExecutionState._StartPending);

	    #if !KTL_USER_MODE
		{
			_ExecutionState._IsCurrentlyDPCDispatch = _ExecutionState._IsDPCDispatch;
			if (ParentAsyncContext)
			{
				//
				// It is not valid for a child to be passive while the parent is DPC dispatch
				//
				KInvariant( ! (! _ExecutionState._IsCurrentlyDPCDispatch &&
							   ParentAsyncContext->_ExecutionState._IsCurrentlyDPCDispatch));
			}
		}
	    #endif

        _ExecutionState._StartPending = TRUE;
        SetStatus(STATUS_PENDING);
        _ParentAsyncContextPtr = ParentAsyncContext;
        _CallbackPtr = CallbackPtr;
        if (!_GlobalContext)
        {
            _GlobalContext = GlobalContext;
        }

        scheduleThread = UnsafeEnsureThread();
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }
}

BOOLEAN
KAsyncContextBase::ScheduleCancel()
{
    BOOLEAN scheduleThread = FALSE;
    BOOLEAN result = FALSE;

    _ThisLock.Acquire();
    if (IsOperatingState() || IsStarting())
    {
        // NOTE: The IsStarting condition allows for a Cancel to be issued imd after the Start()
        //       but before the SM has actually changed state to the Operating state.
        result = TRUE;
        _ExecutionState._CancelPending = TRUE;
        scheduleThread = UnsafeEnsureThread();
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }

    return result;
}

BOOLEAN
KAsyncContextBase::ScheduleComplete(
    __in NTSTATUS CompletionStatus,
    __in_opt CompletingCallback Callback,
    __in void* CallbackContext)
{
    BOOLEAN scheduleThread = FALSE;
    BOOLEAN result = FALSE;

    _ThisLock.Acquire();
    {
        if ((IsOperatingState() || IsStarting()) && !_ExecutionState._CompletePending)
        {
            // Only allow first Complete - ignore the rest
            _ExecutionState._CompletePending = TRUE;
            SetStatus(CompletionStatus);
            scheduleThread = UnsafeEnsureThread();
            result = TRUE;

            if (Callback)
            {
                Callback(CallbackContext);
            }
        }
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }

    return result;
}

BOOLEAN
KAsyncContextBase::ScheduleCompletionDone()
{
    BOOLEAN scheduleThread = FALSE;

    _ThisLock.Acquire();
    {
        KInvariant(IsCompletionPendingState());
        KInvariant(!_ExecutionState._CompletionDonePending);
        _ExecutionState._CompletionDonePending = TRUE;
        scheduleThread = UnsafeEnsureThread();
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        // Special case: The caller's thread is used to dispatch to the last state. This is always a
        // parent KAsyncContextBase because calls to this method by the current instance's state machine
        // would not cause thread scheduling because it would be in a Running state. This effectivly
        // makes the call to this method by a parent sync. This is due to the fact that the parent is
        // not informed of a sub-op completion until that sub-op is in an Idle state - see
        // CompletingStateDispatcher().

#if !KTL_USER_MODE
        if (_ExecutionState._IsCurrentlyDPCDispatch)
        {
            //
            // if this async is DPCDispatch we need to raise irql to dispatch
            //
            KIRQL irql;

            irql= KeRaiseIrqlToDpcLevel();
            Dispatcher();
            KeLowerIrql(irql);
        }
        else
#endif
        {
            Dispatcher();
        }
    }
    return scheduleThread;
}

VOID
KAsyncContextBase::ScheduleLockAcquireCompletion()
{
    BOOLEAN scheduleThread = FALSE;

    InterlockedIncrement(&_LocksHeld);          // (#1) See ReleaseLock()

    _ThisLock.Acquire();
    {
        KInvariant(!_ExecutionState._WaitCompletePending);
        _ExecutionState._WaitCompletePending = TRUE;
        if (_ExecutionState._DispatcherState == WaitingOnAcquireLock)
        {
            _ExecutionState._DispatcherState = Idle;
        }
        scheduleThread = UnsafeEnsureThread();
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }
}

VOID
KAsyncContextBase::ScheduleSubOpCompletion(__in KAsyncContextBase& CompletedSubOpContext)
{
    BOOLEAN scheduleThread = FALSE;

    _ThisLock.Acquire();
    {
        KInvariant(!CompletedSubOpContext.WaitListEntry().IsLinked());
        _CompletionQueue.InsertBottom(CompletedSubOpContext.WaitListEntry());
        scheduleThread = UnsafeEnsureThread();
    }
    _ThisLock.Release();

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }
}

inline
KThreadPool&
KAsyncContextBase::GetThreadPool()
{
    #if KTL_USER_MODE
    {
        KtlSystem&  sys = GetThisKtlSystem();
        return (_ExecutionState._UsingSystemThreadPool ? sys.DefaultSystemThreadPool() : sys.DefaultThreadPool());
    }
    #else
    {
        return GetThisKtlSystem().DefaultThreadPool();
    }
    #endif
}

VOID
KAsyncContextBase::ScheduleThreadPoolThread()
{
    // NOTE: The storage at _WaitListEntrySpace is overload between two different data structures
    //       - LinkListEntry and WorkItem. The space is used as a WorkItem only as long as the
    //       KAsyncContextBase instance is on the ThreadPool work item list waiting for a thread.
    //       To that end, this method formats _WaitListEntrySpace as a WorkItem just before the
    //       instance is queued as a work item (QueueWorkItem()) and then put back into a LinkListEntry
    //       format as the first action when the thread pool dispatches a thread - see Execute() below.

    // Free LinkListEntry stored in _WaitListEntrySpace
    KInvariant(!WaitListEntry().IsLinked());
    FreeWaitListEntry();

    #if !KTL_USER_MODE
        //
        // If this async is one that is designated to run on a DPC
        // thread and we are already at DPC then queue up
        // the DPC for it. If we are not at DPC then do not hijack the
        // current thread to run at DPC.
        //
        if ((_ExecutionState._IsCurrentlyDPCDispatch) && (KeGetCurrentIrql() == DISPATCH_LEVEL))
        {
            BOOLEAN queued;
            queued = KeInsertQueueDpc(_KDPC, NULL, NULL);

            // Do not expect it to already be in the queue
            KInvariant(queued);
            return;
        }
    #endif

    // Format (construct) a WorkItem stored in _WaitListEntrySpace for use by thread pool
    InitializeWorkItemEntry();

    GetThreadPool().QueueWorkItem(WorkItemEntry(), TRUE);
}

#if ! KTL_USER_MODE
// Entry point from DPC dispatch
VOID
KAsyncContextBase::DPCHandler(
    __in PKDPC DPC,
    __in_opt PVOID Context,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KAsyncContextBase* thisPtr = (KAsyncContextBase*)Context;

    KInvariant(thisPtr->_KDPC == DPC);
    KInvariant(thisPtr->_ExecutionState._IsCurrentlyDPCDispatch);

    thisPtr->InitializeWaitListEntry();
    KInvariant(!thisPtr->WaitListEntry().IsLinked());

    thisPtr->Dispatcher();
    // *** Access to 'this' is not safe from this point on given Dispatcher can
    //     indirectly cause destruction of the underlying instance.
}
#endif

// Entry point from threadpool dispatch
VOID
KAsyncContextBase::WorkItem::Execute()
{
    KAsyncContextBase* thisPtr = KAsyncContextBase::KAsyncContextBasePtrFromWorkItemEntry(this);

    // Put LinkListEntry storage back - see ScheduleThreadPoolThread - and then dispatch
    thisPtr->FreeWorkItemEntry();
    thisPtr->InitializeWaitListEntry();
    KInvariant(!thisPtr->WaitListEntry().IsLinked());

    thisPtr->Dispatcher();
    // *** Access to 'this' is not safe from this point on given Dispatcher can
    //     indirectly cause destruction of the underlying instance.
}

// Main dispatcher for a KAsyncContextBase instance
VOID
KAsyncContextBase::Dispatcher()
{
#if !KTL_USER_MODE
    BOOLEAN raisedIrql = FALSE;
    KIRQL oldIrql = 0;

    //
    // If the async is DPC dispatch and we are not at DPC then we need
    // to raise IRQL for the async
    //
    if ((_ExecutionState._IsCurrentlyDPCDispatch) && (KeGetCurrentIrql() < DISPATCH_LEVEL))
    {
        raisedIrql = TRUE;
        oldIrql = KeRaiseIrqlToDpcLevel();
    }
#endif

    BOOLEAN continueDispatching = FALSE;
    do
    {
        continueDispatching = (this->*_CurrentStateDispatcher)(continueDispatching);
    } while (continueDispatching);

#if !KTL_USER_MODE
    if (raisedIrql)
    {
        KeLowerIrql(oldIrql);
    }
#endif
    // *** Access to 'this' is not safe from this point on given CompletedStateDispatcher() can
    //     indirectly cause destruction of the underlying instance.
}

#if KTL_USER_MODE
//** A private imp class used by ReleaseApartment() to communicate that a dispatched thread
//   was hijacked out of the apartment - telling the thread that it has been relieved of
//   it dispatch duties - and that other threads may and can race with it.
class KAsyncContextBase::ApartmentState
{
private:
    friend class KAsyncContextBase;
    BOOLEAN             _ApartmentReleased;
    KAsyncContextBase*  _This;

public:
    __inline ApartmentState(KAsyncContextBase& This)
        :   _This(&This)
    {
        _ApartmentReleased = FALSE;

        // Stuff a pointer to this - so ReleaseApartment() can indicate that it has been called
        KInvariant(This._ApartmentState == nullptr);
        This._ApartmentState = this;
    }

    __inline BOOLEAN
    ApartmentWasReleased()
    {
        if (!_ApartmentReleased)
        {
            KInvariant(_This->_ApartmentState == this);
            _This->_ApartmentState = nullptr;
        }

        _This = nullptr;
        return _ApartmentReleased;
    }

    __inline
    ~ApartmentState()
    {
        KInvariant(_This == nullptr);
    }

private:
    ApartmentState();
};


// API to disassociate the current thread from the current asyn's appt
VOID
KAsyncContextBase::ReleaseApartment()
{
    BOOLEAN scheduleThread = FALSE;

    _ThisLock.Acquire();
    {
        KInvariant(IsOperatingState() || IsCompletingState());
        KInvariant(IsInApartment());
        KInvariant(_ExecutionState._DispatcherState == Running);
        KInvariant((_ApartmentState != nullptr) && (_ApartmentState->_This == this) && !_ApartmentState->_ApartmentReleased);

        // Indicate ReleaseApartment() has been called
        _ApartmentState->_ApartmentReleased = TRUE;

        // Free the appt use
        _ApartmentState = nullptr;
        _ExecutionState._DispatcherState = Idle;
        _CurrentDispatchThreadId = KThread::InvalidThreadId;

        // if work to do, try and schedule a thread
        if (IsWorkPending())
        {
            scheduleThread = UnsafeEnsureThread();
            KInvariant(scheduleThread);
        }
    }
    _ThisLock.Release();

    KInvariant(GetThreadPool().DetachCurrentThread());      // Remove any pending work from the thread - now a free thread

    if (scheduleThread)
    {
        ScheduleThreadPoolThread();
    }
}
#endif


// Dispatchers for each possible state.
BOOLEAN
KAsyncContextBase::InitializedStateDispatcher(__in BOOLEAN Continued)
{
    EventType       currentEvent;
    BOOLEAN         doOnStart = FALSE;

    UNREFERENCED_PARAMETER(Continued);

    do
    {
        _ThisLock.Acquire();
        {
            _ExecutionState._DispatcherState = Running;
            switch (currentEvent = UnsafeGetNextEvent())
            {
                case NoEvent:
                    break;

                case StartEvent:
                    //** Change State
                    doOnStart = TRUE;
                    _CurrentStateDispatcher = &KAsyncContextBase::OperatingStateDispatcher;
                    break;

                default:
                    FailFast();     // Invalid Event type for this state
            }
        }
        _ThisLock.Release();

        if (doOnStart)
        {
            //** Start event received - dispatch to derivation
            _CurrentDispatchThreadId = KThread::GetCurrentThreadId();
            #if KTL_USER_MODE
            {
                ApartmentState  as(*this);
                OnStart();
                if (as.ApartmentWasReleased())
                {
                    // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                    //       could have change many times
                    return FALSE;
                }
            }
            #else
            {
                OnStart();
            }
            #endif
            _CurrentDispatchThreadId = KThread::InvalidThreadId;

            return TRUE;
        }
    } while (currentEvent != NoEvent);

    return FALSE;
}

BOOLEAN
KAsyncContextBase::OperatingStateDispatcher(__in BOOLEAN Continued)
{
    EventType       currentEvent;

    UNREFERENCED_PARAMETER(Continued);

    do
    {
        KAsyncLock*             lockPtr = nullptr;
        LockAcquiredCallback    acquiredLockCallback = nullptr;
        NTSTATUS                waitCompletionStatus = STATUS_SUCCESS;
        KAsyncContextBase*      completedSubOpContextPtr = nullptr;
        BOOLEAN                 lockAcquireComplete = FALSE;

        _ThisLock.Acquire();
        {
            _ExecutionState._DispatcherState = Running;
            switch (currentEvent = UnsafeGetNextEvent())
            {
                case NoEvent:
                    break;

                case CancelEvent:
                    break;

                case CompleteEvent:
                    //** Change State
                    _CurrentStateDispatcher = &KAsyncContextBase::CompletingStateDispatcher;

                    // ensure any pending lock acquire is canceled (!if not owned or pending already!). This has imapct of eating
                    // a would-be alert if this was any event other than a Complete(). The app will not see a callback and thus
                    // won't re-issue what would be a violating Acquire(). This is the right behavior given no lock has been
                    // and we are leaving the operating state.
                    lockAcquireComplete = UnsafeCancelPendingAcquireLock();

                    _ExecutionState._CancelPending = FALSE; // make sure no cancels get into the state machine past this state
                    break;

                case WaitAlertEvent:
                    KInvariant(IsAcquiringLock());
                    lockAcquireComplete = TRUE;
                    waitCompletionStatus = STATUS_ALERTED;
                    break;

                case WaitCompleteEvent:
                    KInvariant(IsAcquiringLock());
                    lockAcquireComplete = TRUE;
                    waitCompletionStatus = STATUS_SUCCESS;
                    break;

                case SubOpCompleteEvent:
                {
                    // ensure any pending lock acquire is canceled (if not owned or pending already)
                    lockAcquireComplete = UnsafeCancelPendingAcquireLock();
                    if (lockAcquireComplete)
                    {
                        // Ignore sub-op completion - it will be picked up again - let the acquire process first
                        waitCompletionStatus = STATUS_ALERTED;
                        currentEvent = WaitCompleteEvent;
                    }
                    else
                    {
                        KInvariant(!_CompletionQueue.IsEmpty());
                        completedSubOpContextPtr = KAsyncContextBasePtrFromWaitListEntry(&(_CompletionQueue.GetTop().Remove()));
                        KInvariant(_PendingWorkCount > 0);
                        _PendingWorkCount--;
                    }
                    break;
                }

                default:
                    FailFast();     // Invalid Event type for this state
            }

            if (lockAcquireComplete)
            {
                KInvariant((currentEvent == WaitAlertEvent) || (currentEvent == WaitCompleteEvent) || (currentEvent == CompleteEvent));

                // Snap lock acquire completion state locally - and release instance state
                lockPtr = _LockPtr;
                _LockPtr = nullptr;

                acquiredLockCallback = _AcquiredLockCallback;
                _AcquiredLockCallback.Reset();

                KInvariant(_PendingWorkCount > 0);
                _PendingWorkCount--;
            }
        }
        _ThisLock.Release();

        switch (currentEvent)
        {
            case CancelEvent:
                //** Dispatch OnCancel()
                _CurrentDispatchThreadId = KThread::GetCurrentThreadId();
                #if KTL_USER_MODE
                {
                    ApartmentState  as(*this);
                    OnCancel();
                    if (as.ApartmentWasReleased())
                    {
                        // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                        //       could have change many times
                        return FALSE;
                    }
                }
                #else
                {
                    OnCancel();
                }
                #endif
                _CurrentDispatchThreadId = KThread::InvalidThreadId;
                break;

            case CompleteEvent:
                return TRUE;         // continue with CompletingStateDispatcher

            case WaitAlertEvent:
            case WaitCompleteEvent:
            {
                if (acquiredLockCallback)
                {
                    //** Invoke derivation callback for lock acquire
                    _CurrentDispatchThreadId = KThread::GetCurrentThreadId();
                    #if KTL_USER_MODE
                    {
                        ApartmentState  as(*this);
#pragma prefast(push)
#pragma prefast(disable:6011, "lockPtr must be non-null given AcquireLock()")
                        acquiredLockCallback(waitCompletionStatus == STATUS_SUCCESS, *lockPtr);
#pragma prefast(pop)
                        if (as.ApartmentWasReleased())
                        {
                            // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                            //       could have change many times
                            return FALSE;
                        }
                    }
                    #else
                    {
#pragma prefast(push)
#pragma prefast(disable:6011, "lockPtr must be non-null given AcquireLock()")
                        acquiredLockCallback(waitCompletionStatus == STATUS_SUCCESS, *lockPtr);
#pragma prefast(pop)
                    }
                    #endif
                    acquiredLockCallback.Reset();
                    _CurrentDispatchThreadId = KThread::InvalidThreadId;
                }

            break;
            }

            case SubOpCompleteEvent:
            {
                KInvariant(!acquiredLockCallback);

                _CurrentDispatchThreadId = KThread::GetCurrentThreadId();   // #1

                // Snap local completion refs and finish completion of the sub operation - must hold ref count past
                // ScheduleCompletionDone() call as it will cause the completing sub operation to release its self-
                // ref count.
#pragma prefast(push)
#pragma prefast(disable:6011, "completedSubOpContextPtr must be non-null given SubOpCompleteEvent logic in above switch statement")
                CompletionCallback      callbackPtr = completedSubOpContextPtr->_CallbackPtr;
#pragma prefast(pop)
                SPtr                    completedSubOpContextSPtr(completedSubOpContextPtr);

#pragma prefast(push)
#pragma prefast(disable:6011, "completedSubOpContextPtr must be non-null given SubOpCompleteEvent logic in above switch statement")
                if (!completedSubOpContextPtr->ScheduleCompletionDone())
#pragma prefast(pop)
                {
                    // For some reason the state machine (SM) of completedSubOpContextPtr was not in the right state and thus
                    // the ScheduleCompletionDone did not execute and guarantee that SM IsCompletedState() - meaning it would
                    // not be valid to call the callback.
                    FailFast();
                }

                #if !KTL_USER_MODE
                KIRQL irql = 0;

#pragma prefast(push)
#pragma prefast(disable:6011, "completedSubOpContextPtr must be non-null given SubOpCompleteEvent logic in above switch statement")
                if (completedSubOpContextPtr->_ExecutionState._IsCurrentlyDPCDispatch)
#pragma prefast(pop)
                {
                    irql = KeRaiseIrqlToDpcLevel();
                }
                #endif

                completedSubOpContextPtr->OnCompleted();
                if (callbackPtr)
                {
                    #if KTL_USER_MODE
                    {
                        ApartmentState  as(*this);
                        callbackPtr(this, *completedSubOpContextPtr);
                        if (as.ApartmentWasReleased())
                        {
                            // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                            //       could have change many times
                            return FALSE;
                        }
                    }
                    #else
                    {
                        callbackPtr(this, *completedSubOpContextPtr);
                    }
                    #endif
                }

                #if !KTL_USER_MODE
                if (completedSubOpContextPtr->_ExecutionState._IsCurrentlyDPCDispatch)
                {
                    KeLowerIrql(irql);
                }
                #endif

                completedSubOpContextPtr = nullptr;
                completedSubOpContextSPtr.Reset();

                _CurrentDispatchThreadId = KThread::InvalidThreadId;        // #1
                break;
            }
        }
    } while (currentEvent != NoEvent);

    return FALSE;
}

BOOLEAN
KAsyncContextBase::CompletingStateDispatcher(__in BOOLEAN Continued)
{
    // NOTE: This state behaves just like the OperatingState in terms of completion
    //       callbacks being invoked. The difference is Complete() and Cancel() calls
    //       will not be accepted while in this state.
    //
    //       This state is only exit when _PendingWorkCount == 0.
    //
    EventType           currentEvent;

    UNREFERENCED_PARAMETER(Continued);

    do
    {
        BOOLEAN                 doStateChange = FALSE;
        NTSTATUS                waitCompletionStatus = STATUS_SUCCESS;
        BOOLEAN                 lockAcquireComplete = FALSE;
        KAsyncLock*             lockPtr = nullptr;
        KAsyncContextBase*      completedSubOpContextPtr = nullptr;
        LockAcquiredCallback    acquiredLockCallback = nullptr;

        _ThisLock.Acquire();
        {
            _ExecutionState._DispatcherState = Running;
            switch (currentEvent = UnsafeGetNextEvent())
            {
                case NoEvent:
                    if ((doStateChange = (_PendingWorkCount == 0)) == TRUE)
                    {
                        // Override UnsafeGetNextEvent and keep the dispatcher locked
                        _ExecutionState._DispatcherState = Running;

                        //** Change State
                        _CurrentStateDispatcher = &KAsyncContextBase::CompletionPendingStateDispatcher;
                    }
                    break;

                case WaitCompleteEvent:
                    KInvariant(IsAcquiringLock());
                    lockAcquireComplete = TRUE;
                    waitCompletionStatus = STATUS_SUCCESS;
                    break;

                case SubOpCompleteEvent:
                {
                    // ensure any pending lock acquire is canceled (if not owned or pending already)
                    lockAcquireComplete = UnsafeCancelPendingAcquireLock();
                    if (lockAcquireComplete)
                    {
                        // Ignore sub-op completion - it will be picked up again - let the acquire process first
                        waitCompletionStatus = STATUS_ALERTED;
                        currentEvent = WaitCompleteEvent;
                    }
                    else
                    {
                        KInvariant(!_CompletionQueue.IsEmpty());
                        completedSubOpContextPtr = KAsyncContextBasePtrFromWaitListEntry(&(_CompletionQueue.GetTop().Remove()));
                        KInvariant(_PendingWorkCount > 0);
                        _PendingWorkCount--;
                    }
                    break;
                }

                default:
                    FailFast();     // Invalid Event type for this state
            }

            if (lockAcquireComplete)
            {
                KInvariant(currentEvent == WaitCompleteEvent);

                // Snap lock acquire completion state locally - and release instance state
                lockPtr = _LockPtr;
                _LockPtr = nullptr;

                acquiredLockCallback = _AcquiredLockCallback;
                _AcquiredLockCallback.Reset();

                KInvariant(_PendingWorkCount > 0);
                _PendingWorkCount--;
            }
        }
        _ThisLock.Release();

        switch (currentEvent)
        {
            case WaitCompleteEvent:
                if (acquiredLockCallback)
                {
                    //** Invoke derivation callback for lock acquire
                    _CurrentDispatchThreadId = KThread::GetCurrentThreadId();
                    #if KTL_USER_MODE
                    {
                        ApartmentState  as(*this);
#pragma prefast(push)
#pragma prefast(disable:6011, "lockPtr must be non-null given AcquireLock()")
                        acquiredLockCallback(waitCompletionStatus == STATUS_SUCCESS, *lockPtr);
#pragma prefast(pop)
                        if (as.ApartmentWasReleased())
                        {
                            // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                            //       could have change many times
                            return FALSE;
                        }
                    }
                    #else
                    {
#pragma prefast(push)
#pragma prefast(disable:6011, "lockPtr must be non-null given AcquireLock()")
                        acquiredLockCallback(waitCompletionStatus == STATUS_SUCCESS, *lockPtr);
#pragma prefast(pop)
                    }
                    #endif
                    acquiredLockCallback.Reset();
                    _CurrentDispatchThreadId = KThread::InvalidThreadId;
                }
                break;

            case SubOpCompleteEvent:
            {
                KInvariant(!acquiredLockCallback);

                _CurrentDispatchThreadId = KThread::GetCurrentThreadId();   // #1

                // Snap local completion refs and finish completion of the sub operation - must hold ref count past
                // ScheduleCompletionDone() call as it will cause the completing sub operation to release its self-
                // ref count.
#pragma prefast(push)
#pragma prefast(disable:6011, "completedSubOpContextPtr must be non-null given SubOpCompleteEvent logic in above switch statement")
                CompletionCallback      callbackPtr = completedSubOpContextPtr->_CallbackPtr;
#pragma prefast(pop)
                SPtr                    completedSubOpContextSPtr(completedSubOpContextPtr);

                if (!completedSubOpContextPtr->ScheduleCompletionDone())
                {
                    // For some reason the state machine (SM) of completedSubOpContextPtr was not in the right state and thus
                    // the ScheduleCompletionDone did not execute and guarantee that SM IsCompletedState() - meaning it would
                    // not be valid to call the callback.
                    FailFast();
                }

                #if !KTL_USER_MODE
                KIRQL irql = 0;

                if (completedSubOpContextPtr->_ExecutionState._IsCurrentlyDPCDispatch)
                {
                    irql = KeRaiseIrqlToDpcLevel();
                }
                #endif

                completedSubOpContextPtr->OnCompleted();
                if (callbackPtr)
                {
                    #if KTL_USER_MODE
                    {
                        ApartmentState  as(*this);
                        callbackPtr(this, *completedSubOpContextPtr);
                        if (as.ApartmentWasReleased())
                        {
                            // NOTE: "this" can't be assumed to be stable if the appt was released. The state of "this"
                            //       could have change many times
                            return FALSE;
                        }
                    }
                    #else
                    {
                        callbackPtr(this, *completedSubOpContextPtr);
                    }
                    #endif
                }

                #if !KTL_USER_MODE
                if (completedSubOpContextPtr->_ExecutionState._IsCurrentlyDPCDispatch)
                {
                    KeLowerIrql(irql);
                }
                #endif

                completedSubOpContextPtr = nullptr;
                completedSubOpContextSPtr.Reset();

                _CurrentDispatchThreadId = KThread::InvalidThreadId;        // #1
                break;
            }
        }

        if (doStateChange)
        {
            KInvariant(_PendingWorkCount == 0);
            KInvariant(_ExecutionState._DispatcherState == Running);

            if (_LocksHeld > 0)
            {
                // Derivation must have released all held KAsyncLocks at this point
                HandleOutstandingLocksHeldAtComplete(); // Defaults to fail fast
            }

            if (_ParentAsyncContextPtr == nullptr)
            {
                // For the case where there is no associated parent AsyncContext
                if (ScheduleCompletionDone())
                {
                    // ScheduleCompletionDone() indicated that it dispatched the CompletionDone event - which should
                    // never happen given that _ExecutionState._DispatcherState == Running (see NoEvent case above).
                    FailFast();
                }
            }
            return TRUE;        // do state change but keep dispatching
        }
    } while (currentEvent != NoEvent);

    return FALSE;
}

VOID
KAsyncContextBase::HandleOutstandingLocksHeldAtComplete()
{
    // NOTE: Only intented to be overridden by KAsyncLockContext implementation
    FailFast();
}

BOOLEAN
KAsyncContextBase::CompletionPendingStateDispatcher(__in BOOLEAN Continued)
{
    EventType       currentEvent;
    do
    {
        _ThisLock.Acquire();
        {
            _ExecutionState._DispatcherState = Running;
            switch (currentEvent = UnsafeGetNextEvent())
            {
                case NoEvent:
                    break;

                case CompletionDoneEvent:
                    //** Change State
                    _CurrentStateDispatcher = &KAsyncContextBase::CompletedStateDispatcher;
                    break;

                default:
                    FailFast();         // Invalid Event type for this state
            }
        }
        _ThisLock.Release();

        if (currentEvent == CompletionDoneEvent)
        {
            return TRUE;                // do state change but keep dispatching
        }
    } while (currentEvent != NoEvent);

    if (Continued && (_ParentAsyncContextPtr != nullptr))
    {
        // We have drained the "event queue" on entry into this state and we now inform the parent AsyncContext this
        // sub-op is complete. By delaying this call till this point, we guarentee that the parent AsyncContext will
        // provide its thread to finish the completion sequence on this sub-op before the parent's completion callback
        // for this sub-op is called.
        // See: ScheduleCompletionDone()
        _ParentAsyncContextPtr->ScheduleSubOpCompletion(*this);
    }

    return FALSE;
}

BOOLEAN
KAsyncContextBase::CompletedStateDispatcher(__in BOOLEAN Continued)
{
    // NOTE: The only way out of this state is via a Reuse() call
    EventType       currentEvent;

    do
    {
        _ThisLock.Acquire();
        {
            _ExecutionState._DispatcherState = Running;
            switch (currentEvent = UnsafeGetNextEvent())
            {
                case NoEvent:
                    break;

                default:
                    FailFast();     // Invalid Event type for this state
            }
        }
        _ThisLock.Release();

    } while (currentEvent != NoEvent);

    if (Continued)
    {
        // On entry to this state (Continued == true)...
        if (_ParentAsyncContextPtr == nullptr)
        {
            //** No parent KAsyncContextBase - allow for simple completion callback
            CompletionCallback  callbackPtr = _CallbackPtr;
            _CallbackPtr.Reset();

            KThread::Id currentThreadId = KThread::GetCurrentThreadId();
            _CurrentDispatchThreadId    = currentThreadId;

            OnCompleted();
            _GlobalContext.Reset();

            if (callbackPtr)
            {
                #if KTL_USER_MODE
                {
                    if (_ExecutionState._DetachThreadOnCompletion)
                    {
                        KInvariant(GetThreadPool().DetachCurrentThread());      // Remove any pending work from the thread - now a free thread
                    }
                }
                #endif

                callbackPtr(nullptr, *this);
            }

            // Only invalidate the current Apt thread if the callbackPtr() call above did not cause a new thread to
            // be dispatched into this instance due to a Reuse(), Start*() sequence in that callback.
            InterlockedCompareExchange64((LONGLONG*)&_CurrentDispatchThreadId, KThread::InvalidThreadId, currentThreadId);
        }
        else
        {
            // Parent will handle any callbacks... See: (Operating/Completing)StateDispatcher::SubOpCompleteEvent
            _ParentAsyncContextPtr = nullptr;
            _CallbackPtr.Reset();
            _GlobalContext.Reset();
        }

        PrivateOnFinalize();    // Finish completion sequence allowing a friend derivation to override.
                                // Such a friend derivation MUST call __super::PrivateOnFinalize().

        // ** NOTE: 'this' can't be assumed to be valid beyond this point given the underlying
        //          instance could have been destructed indirectly by PrivateOnFinalize().
    }
    return FALSE;
}

VOID
KAsyncContextBase::PrivateOnFinalize()
{
    Release();          // See #1 in Start()
}

KAsyncContextBase::EventType
KAsyncContextBase::UnsafeGetNextEvent()
{
    // KAssert(_ThisLock.IsHeld());
    if (_ExecutionState._StartPending)                  // NOTE: StartPending MUST be decoded before all other
    {                                                   //       to guarentee the SM is in the Operating state
        _ExecutionState._StartPending = FALSE;          //       before any other events will be processed
        return StartEvent;
    }
    else if (_ExecutionState._CompletePending)
    {
        _ExecutionState._CompletePending = FALSE;
        return CompleteEvent;
    }
    else if (!_CompletionQueue.IsEmpty())
    {
        return SubOpCompleteEvent;
    }
    else if (_ExecutionState._WaitCompletePending)
    {
        _ExecutionState._WaitCompletePending = FALSE;
        _ExecutionState._AcquireLockRequested = FALSE;      // We saw a quick lock acquire
        return WaitCompleteEvent;
    }
    else if (_ExecutionState._AlertCompletePending)
    {
        _ExecutionState._AlertCompletePending = FALSE;
        return WaitAlertEvent;
    }
    else if (_ExecutionState._CancelPending)
    {
        _ExecutionState._CancelPending = FALSE;
        return CancelEvent;
    }
    else if (_ExecutionState._CompletionDonePending)
    {
        _ExecutionState._CompletionDonePending = FALSE;
        return CompletionDoneEvent;
    }

    // All events drained - move the Dispatcher state to either waiting for acquire OR Idle
    if (_ExecutionState._AcquireLockRequested)
    {
        _ExecutionState._AcquireLockRequested = FALSE;
        _ExecutionState._DispatcherState = WaitingOnAcquireLock;
    }
    else
    {
        _ExecutionState._DispatcherState = Idle;
    }
    return NoEvent;
}

NTSTATUS
KAsyncContextBase::CallbackAsyncContext(
    __in ULONG AllocationTag,
    __in CompletionCallback Completion)
/*++
 *
 * Routine Description:
 *      This routine generates a callback in the current KAsyncContextBase apartment.
 *      It allocates and starts an empty context which completes immedately.
 *
 * Arguments:
 *      Completion - The requestors callback routine.
 *
 * Return Value:
 *      Normally STATUS_PENDING
 *
 * Note:
 *
-*/
{
    KAsyncContextBase::SPtr context = _new(AllocationTag, GetThisAllocator()) KAsyncContextBase;

    if (!context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = context->Status();
    if (!NT_SUCCESS(status))
    {
        context.Reset();     // Cause destruction of object just created.
        return status;
    }

    #if !KTL_USER_MODE
    {
        //
        // If our async is DPCDispatch then the child that kicks us must
        // also be DPCDispatch
        //
        if (_ExecutionState._IsCurrentlyDPCDispatch)
        {
            context->SetDispatchOnDPCThread(TRUE);
        }
    }
    #endif

    //
    // This context will complete immediately
    //
    context->Start(this, Completion);
    return(STATUS_PENDING);
}

//* Common support methods
VOID
KAsyncContextBase::FailFast()
{
    // BUG, richhas, xxxxxx, use real FailFast primitive
    KInvariant(FALSE);
    UCHAR f = *((UCHAR*)0);
    f++;
}


//*** KAsyncLock implemenation
KAsyncLock::KAsyncLock()
{
}

KAsyncLock::~KAsyncLock()
{
    KInvariant(!_OwnerSPtr);
}

BOOLEAN
KAsyncLock::AcquireLock(__in KAsyncContextBase* RequesterPtr)
{
    BOOLEAN acquired = FALSE;

    _ThisLock.Acquire();
    {
        if (_OwnerSPtr)
        {
            KInvariant(_OwnerSPtr.RawPtr() != RequesterPtr);   // Re-acquire not allowed
            _Waiters.InsertBottom(RequesterPtr->WaitListEntry());
            RequesterPtr->AddRef();             // #1
        }
        else
        {
            _OwnerSPtr = RequesterPtr;
            acquired = TRUE;
        }
    }
    _ThisLock.Release();

    return acquired;
}

VOID
KAsyncLock::ReleaseLock(__in KAsyncContextBase* RequesterPtr)
{
    KAsyncContextBase::SPtr oldOwner;
    KAsyncContextBase* newOwner = nullptr;

    _ThisLock.Acquire();
    {
        // BUG, richhas, xxxxx, these next two lines should be replaced by an Detach/Attach() when available
        oldOwner = _OwnerSPtr;
        _OwnerSPtr.Reset();
        KInvariant(oldOwner.RawPtr() == RequesterPtr);     // Requester must be the owner
        #if !DBG
        UNREFERENCED_PARAMETER(RequesterPtr);
        #endif
        if (!_Waiters.IsEmpty())
        {
            // Take ref to listed waiter: see #1 in Acquire()
            // BUG, richhas, xxxxx, these next two lines should be replaced by an Attach() when available
            _OwnerSPtr = KAsyncContextBase::KAsyncContextBasePtrFromWaitListEntry(&(_Waiters.GetTop().Remove()));
            newOwner = _OwnerSPtr.RawPtr();
        }
    }
    _ThisLock.Release();

    if (newOwner)
    {
        // Inform new owner that it now owns the Lock - complete it's Acquire
        newOwner->ScheduleLockAcquireCompletion();

        // Release the ref at #1 in Acquire()
        newOwner->Release();
    }
}

BOOLEAN
KAsyncLock::IsLockOwner(__in KAsyncContextBase* RequesterPtr)
{
    BOOLEAN isOwner = FALSE;

    _ThisLock.Acquire();
    {
        if (_OwnerSPtr.RawPtr() == RequesterPtr)
        {
            isOwner = TRUE;
        }
        else
        {
            isOwner = FALSE;
        }
    }
    _ThisLock.Release();

    return isOwner;
}

BOOLEAN
KAsyncLock::CancelAcquire(__in KAsyncContextBase* RequesterPtr)     // Returns TRUE if *RequesterPtr was Canceled
{
    // ASSUMPTION: RequesterPtr->_ThisLock is held by caller
    LinkListEntry*  requesterLinkEntryPtr = &RequesterPtr->WaitListEntry();
    BOOLEAN         doCancel = FALSE;

    _ThisLock.Acquire();
    {
        if ((((doCancel = RequesterPtr->UnsafeIsPendingLockAcquire(*this)) == TRUE)
            && !(_OwnerSPtr && (_OwnerSPtr.RawPtr() == RequesterPtr))))

        {
            // RequesterPtr has pending lock acquire to this instance and this instance is not yet
            // owned by RequesterPtr - pull from the waiters list.
            KInvariant(requesterLinkEntryPtr->IsLinked());
            requesterLinkEntryPtr->Remove();
        }
    }
    _ThisLock.Release();

    if (doCancel)
    {
        // Release ref to listed waiter: see #1 in Acquire()
        RequesterPtr->Release();
        RequesterPtr = nullptr;
    }

    return doCancel;
}


//** KSharedAsyncLock::Handle implementation
KSharedAsyncLock::Handle::Handle()
{
}

KSharedAsyncLock::Handle::~Handle()
{
}

// Private imp class for KSharedAsyncLock::Handle
class KAsyncLockHandleImp : public KSharedAsyncLock::Handle
{
    K_FORCE_SHARED(KAsyncLockHandleImp);

public:
    KAsyncLockHandleImp(KSharedAsyncLock::SPtr Lock);

    VOID
    StartAcquireLock(
        KAsyncContextBase* const Parent,
        KAsyncContextBase::CompletionCallback Callback,
        KAsyncGlobalContext* const GlobalContext);

    VOID ReleaseLock();

private:
    VOID OnStart();
    VOID OnReuse();
    VOID LocalLockAcquiredCallback(BOOLEAN IsAcquired, KAsyncLock& Lock);
    VOID HandleOutstandingLocksHeldAtComplete();

private:
    KSharedAsyncLock::SPtr  _Lock;
    volatile BOOLEAN        _LockIsHeld;
    LockAcquiredCallback    _Callback;
};

NTSTATUS
KSharedAsyncLock::Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out KSharedAsyncLock::SPtr& Result)
//
// Public API - KAsyncLock.h - KSharedAsyncLock Factory
//
{
    Result = _new(AllocationTag, Allocator) KSharedAsyncLock();
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();     // Cause destruction of object just created.
    }

    return status;
}

KSharedAsyncLock::KSharedAsyncLock()
{
}

KSharedAsyncLock::~KSharedAsyncLock()
{
}

NTSTATUS
KSharedAsyncLock::CreateHandle(
    __in ULONG AllocationTag,
    __out Handle::SPtr& Result)
//
// Public API - KAsyncLock.h - KSharedAsyncLock::Handle Factory
//
{
    Result = _new(AllocationTag, GetThisAllocator()) KAsyncLockHandleImp(KSharedAsyncLock::SPtr(this));
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();     // Cause destruction of object just created.
    }

    return status;
}

//* KAsyncLockHandleImp implementation
KAsyncLockHandleImp::KAsyncLockHandleImp(KSharedAsyncLock::SPtr Lock)
    :   _Lock(Ktl::Move(Lock)),
        _LockIsHeld(FALSE)
{
    _Callback.Bind(this, &KAsyncLockHandleImp::LocalLockAcquiredCallback);
}

KAsyncLockHandleImp::~KAsyncLockHandleImp()
{
    KInvariant(!_LockIsHeld);
    _Lock.Reset();
}

VOID
KAsyncLockHandleImp::HandleOutstandingLocksHeldAtComplete()
//
// NOTE: This override allows this class to complete with locks still being held.
//       See KAsyncContextBase. Only this class is allowed to override
//       HandleOutstandingLocksHeldAtComplete().
{
}

VOID
KAsyncLockHandleImp::StartAcquireLock(
    KAsyncContextBase* const Parent,
    KAsyncContextBase::CompletionCallback Callback,
    KAsyncGlobalContext* const GlobalContext)
//
// Public API - KAsyncLock.h
//
{
    Start(Parent, Callback, GlobalContext);
}

VOID
KAsyncLockHandleImp::OnStart()
//
// Continuation from StartAcquireLock()
//
{
    KInvariant(!_LockIsHeld);                       // Caller did not call ReleaseLock();
    AcquireLock(*_Lock, _Callback);
}

VOID
KAsyncLockHandleImp::LocalLockAcquiredCallback(BOOLEAN IsAcquired, KAsyncLock& Lock)
//
// Continuation from OnStart()
//
// Called either because the _Lock was acquired OR Cancel() was called. A call to
// Cancel() (even without an OnCancel() imp) will cause a pending acquire to be
// alerted and thus IsAcquired will be FALSE.
//
{
    KAssert(&Lock == static_cast<KAsyncLock*>(&(*_Lock)));
#if !DBG
    UNREFERENCED_PARAMETER(Lock);
#endif

    KAssert(!_LockIsHeld);

    _LockIsHeld = IsAcquired;
    if (IsAcquired)
    {
        Complete(STATUS_SUCCESS);
    }
    else
    {
        // Would be caused because Cancel() was called when we are waiting for the lock
        Complete(STATUS_CANCELLED);
    }
}

VOID
KAsyncLockHandleImp::OnReuse()
{
    // Caller did not call ReleaseLock();
    KInvariant(!_LockIsHeld);
}

VOID
KAsyncLockHandleImp::ReleaseLock()
//
// Public API - KAsyncLock.h
//
{
    KAssert(_LockIsHeld);
    KAsyncContextBase::ReleaseLock(*_Lock);     // KAsyncContextBase::ReleaseLock will FF if lock not held
    _LockIsHeld = FALSE;
}


//*** KAsyncGlobalContext imp
KAsyncGlobalContext::KAsyncGlobalContext()
    : _ActivityId(0)
{
}

KAsyncGlobalContext::KAsyncGlobalContext( KActivityId ActivityId)
    : _ActivityId(ActivityId)

{
}

KActivityId
KAsyncGlobalContext::GetActivityId(
    __in_opt KAsyncGlobalContext::SPtr GlobalContext
    )
/*++
 *
 * Routine Description:
 *      Gets the activity id an KAsyncGlobalContext.
 *
 * Arguments:
 *      GlobalContext - Pointer to an KGlobalContext which may be null.
 *
 * Return Value:
 *      Returns the Activity id in the global context or zero.
 *
 * Note:
 *      This is a static method becuase the GlobalContext can be null.
-*/
{

    if(GlobalContext == nullptr)
    {
        return(0);
    }

    return(GlobalContext->_ActivityId);
}

KAsyncGlobalContext::~KAsyncGlobalContext()
{
}


//*** KAsyncContextInterceptor implementation
NTSTATUS
KAsyncContextInterceptor::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KSharedPtr<KAsyncContextInterceptor>& Context)
{
    Context = _new(AllocationTag, Allocator) KAsyncContextInterceptor();
    if (Context == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

KAsyncContextInterceptor::KAsyncContextInterceptor()
{
}

KAsyncContextInterceptor::~KAsyncContextInterceptor()
{
    KInvariant(_InterceptedAsync == nullptr);
}


KGlobalSpinLockStorage KAsyncContextInterceptor::gs_GlobalLock;

// Implement a "global" list of KAsyncContextInterceptors
KNodeList<KAsyncContextInterceptor>*
KAsyncContextInterceptor::UnsafeGetKAsyncContextInterceptors()
{
    KAssert(gs_GlobalLock.Lock().IsOwned());

    static BOOLEAN volatile     isInitialized = FALSE;
    static UCHAR                listStorage[sizeof(KNodeList<KAsyncContextInterceptor>)];

    KNodeList<KAsyncContextInterceptor>*   result = (KNodeList<KAsyncContextInterceptor>*)&listStorage[0];

    if (!isInitialized)
    {
        // Ctor in-place the list header first time thru
        result->KNodeList<KAsyncContextInterceptor>::KNodeList(FIELD_OFFSET(KAsyncContextInterceptor, _InterceptorsListEntry));
        isInitialized = TRUE;
    }

    return result;
}

KAsyncContextInterceptor*
KAsyncContextInterceptor::FindInterceptor(KAsyncContextBase& ForIntercepted)
{
    KNodeList<KAsyncContextInterceptor>*    list = nullptr;
    KAsyncContextInterceptor*               nextEntry = nullptr;

    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        list = UnsafeGetKAsyncContextInterceptors();
        nextEntry = list->PeekHead();
        while (nextEntry != nullptr)
        {
            if (nextEntry->_InterceptedAsync == &ForIntercepted)
            {
                return nextEntry;
            }
            nextEntry = list->Successor(nextEntry);
        }
    }

    return nullptr;
}

VOID
KAsyncContextInterceptor::EnableIntercept(KAsyncContextBase& OnAsync)
{
    KInvariant(OnAsync.Status() == K_STATUS_NOT_STARTED);


    AddRef();           // #1 - Keep the instance alive while interception is active
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KInvariant((_InterceptorsListEntry.Flink == nullptr) && (_InterceptorsListEntry.Blink == nullptr));

        // Save OnAsync's v-table ptr; and insert this class's in it's place
        _SavedInterceptedVTablePtr = *((void**)(&OnAsync));
        *((void**)(&OnAsync)) = *((void**)this);

        _InterceptedAsync = &OnAsync;
        UnsafeGetKAsyncContextInterceptors()->AppendTail(this);
    }

	//
	// Extract callback addresses for the underlying async so we can forward calls to them
	//
	PVOID* vtableArray = (PVOID*)_SavedInterceptedVTablePtr;

	PVOID onStartCallback = (vtableArray[OnStartVtableOffset]);
	_OnStartCallback = (OnStartCallback)onStartCallback;

	PVOID onCancelCallback = (vtableArray[OnCancelVtableOffset]);
	_OnCancelCallback = (OnCancelCallback)onCancelCallback;

	PVOID onReuseCallback = (vtableArray[OnReuseVtableOffset]);
	_OnReuseCallback = (OnReuseCallback)onReuseCallback;

	PVOID onCompletedCallback = (vtableArray[OnCompletedVtableOffset]);
	_OnCompletedCallback = (OnStartCallback)onCompletedCallback;
}

VOID
KAsyncContextInterceptor::DisableIntercept()
{
    KInvariant(_InterceptedAsync->Status() != STATUS_PENDING);

    // Put back the intercepted async's vtable ptr and remove this interceptor from the global list
    K_LOCK_BLOCK(gs_GlobalLock.Lock())
    {
        KInvariant((_InterceptorsListEntry.Flink != nullptr) && (_InterceptorsListEntry.Blink != nullptr));

        *((void**)_InterceptedAsync.RawPtr()) = _SavedInterceptedVTablePtr;
        _SavedInterceptedVTablePtr = nullptr;
        _InterceptedAsync = nullptr;

        UnsafeGetKAsyncContextInterceptors()->Remove(this);
    }
    Release();      // Reverse #1 in EnableIntercept
}

VOID
KAsyncContextInterceptor::ForwardOnStart()
{
	KAsyncContextBase* async = _InterceptedAsync.RawPtr();
	(_OnStartCallback)(async);
}

VOID
KAsyncContextInterceptor::ForwardOnCancel()
{
	KAsyncContextBase* async = _InterceptedAsync.RawPtr();
	(_OnCancelCallback)(async);
}

VOID
KAsyncContextInterceptor::ForwardOnReuse()
{
	KAsyncContextBase* async = _InterceptedAsync.RawPtr();
	(_OnReuseCallback)(async);
}

VOID
KAsyncContextInterceptor::ForwardOnCompleted()
{
	KAsyncContextBase* async = _InterceptedAsync.RawPtr();
	(_OnCompletedCallback)(async);
}

VOID
KAsyncContextInterceptor::OnStart()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    // Find the real "this"
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);
    KInvariant(realThis != nullptr);
    KAssert(this == realThis->_InterceptedAsync.RawPtr());

    realThis->OnStartIntercepted();
}

VOID
KAsyncContextInterceptor::OnCancel()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    // Find the real "this"
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);
    KInvariant(realThis != nullptr);
    KAssert(this == realThis->_InterceptedAsync.RawPtr());

    realThis->OnCancelIntercepted();
}

VOID
KAsyncContextInterceptor::OnCompleted()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    // Find the real "this"
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);
    KInvariant(realThis != nullptr);
    KAssert(this == realThis->_InterceptedAsync.RawPtr());

    realThis->OnCompletedIntercepted();
}

VOID
KAsyncContextInterceptor::OnReuse()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    // Find the real "this"
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);
    KInvariant(realThis != nullptr);
    KAssert(this == realThis->_InterceptedAsync.RawPtr());

    realThis->OnReuseIntercepted();
}

VOID
KAsyncContextInterceptor::HandleOutstandingLocksHeldAtComplete()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);
    KInvariant(realThis != nullptr);
    KAssert(this == realThis->_InterceptedAsync.RawPtr());

    AddRef();
    {
        K_LOCK_BLOCK(gs_GlobalLock.Lock())
        {
            if (realThis->_SavedInterceptedVTablePtr != nullptr)
            {
                *((void**)this) = realThis->_SavedInterceptedVTablePtr;
            }
        }
        this->HandleOutstandingLocksHeldAtComplete();

        K_LOCK_BLOCK(gs_GlobalLock.Lock())
        {
            if (realThis->_SavedInterceptedVTablePtr != nullptr)
            {
                *((void**)this) = *((void**)realThis);
            }
        }
    }
    Release();
}

// This method support any "special" classes that trap PrivateOnFinalize - very few.
VOID
KAsyncContextInterceptor::PrivateOnFinalize()
{
    // NOTE: the 'this' is not pointing to a KAsyncContextInterceptor
    KAsyncContextInterceptor*   realThis = FindInterceptor(*this);

    // NOTE: the call to this method can race with calls to DisableIntercept() so realThis
    //       could be null
    AddRef();
    {
        K_LOCK_BLOCK(gs_GlobalLock.Lock())
        {
            if ((realThis != nullptr) && (realThis->_SavedInterceptedVTablePtr != nullptr))
            {
                KAssert(this == realThis->_InterceptedAsync.RawPtr());
                *((void**)this) = realThis->_SavedInterceptedVTablePtr;
            }
        }
        this->PrivateOnFinalize();

        K_LOCK_BLOCK(gs_GlobalLock.Lock())
        {
            if ((realThis != nullptr) && (realThis->_SavedInterceptedVTablePtr != nullptr))
            {
                *((void**)this) = *((void**)realThis);
            }
        }
    }
    Release();
}

//** KTL++ Support
#if defined(K_UseResumable)
//** CTOR/DTOR for StartAwaiter
namespace ktl 
{
    namespace kasync 
    {
        InplaceStartAwaiter::~InplaceStartAwaiter()
        {
            KInvariant(!_awaiterHandle);
        }

        StartAwaiter::~StartAwaiter()
        {
        }
    }

#if defined(K_UseResumableDbgExt)
    KGlobalSpinLockStorage      Task::promise_type::gs_GlobalLock;
    LinkListHeader              Task::promise_type::gs_List;

    KGlobalSpinLockStorage      PromiseCommon::gs_GlobalLock;
    LinkListHeader              PromiseCommon::gs_List;

    void Task::EnumActiveTasks(EnumCallback Callback)
    {
        K_LOCK_BLOCK(promise_type::gs_GlobalLock.Lock())
        {
            LinkListEntry*  next = &promise_type::gs_List.GetTop();
            while (!promise_type::gs_List.IsEndOfList(*next))
            {
                promise_type* pt = (promise_type*)(((char*)next) - offsetof(promise_type, _ListEntry));
                Callback(*pt);
                next = &next->GetNext();
            }
        }
    }

    void PromiseCommon::EnumActiveAwaitables(EnumCallback Callback)
    {
        K_LOCK_BLOCK(PromiseCommon::gs_GlobalLock.Lock())
        {
            LinkListEntry*  next = &PromiseCommon::gs_List.GetTop();
            while (!PromiseCommon::gs_List.IsEndOfList(*next))
            {
                PromiseCommon* pt = (PromiseCommon*)next;
                Callback(*pt);
                next = &next->GetNext();
            }
        }
    }
#endif
}
#endif
