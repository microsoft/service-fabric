/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KAsyncContextBase.h

    Description:
      Kernel Template Library (KTL): Async Continuation Support Definitions

    History:
      richhas          23-Sep-2010         Initial version.
      richhas          01-Aug-2016         Added Co-routine support

--*/

#pragma once

class KAsyncLock;


//
// The following class defines the type used to identify transactions or related
// events accross components.
//

class KActivityId : public KStrongType<KActivityId, ULONGLONG>
{
    K_STRONG_TYPE(KActivityId, ULONGLONG);

public:

    KActivityId()
    {
        _Value = 0;
    }
};

//** KAsyncContextBase related definitions

const ULONGLONG KAsyncGlobalContextTypeName = K_MAKE_TYPE_TAG('txtG', 'nysA');

class KAsyncGlobalContext abstract: public KObject<KAsyncGlobalContext>, public KShared<KAsyncGlobalContext>, public KTagBase<KAsyncGlobalContext, KAsyncGlobalContextTypeName>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncGlobalContext);

    const KActivityId _ActivityId;

protected:

    KAsyncGlobalContext( KActivityId ActitityId);

public:

    //
    // Becuase GlobalContext is generally optional and therefore sometimes NULL
    // ActivityId must be accessed via this static function.
    //

    static KActivityId
    GetActivityId( __in_opt KAsyncGlobalContext::SPtr const GlobalContext );

};

const ULONGLONG KAsyncContextBaseTypeName = K_MAKE_TYPE_TAG('esaB', 'nysA');

class KAsyncContextBase : public KObject<KAsyncContextBase>, public KShared<KAsyncContextBase>, public KTagBase<KAsyncContextBase, KAsyncContextBaseTypeName>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncContextBase);

public:
    typedef KSharedPtr<KAsyncContextBase> SPtr;

    // Override of KObject<>Status()
    //
    // Returns: Status of the object. If some status other than K_STATUS_NOT_STARTED or STATUS_PENDING,
    //          means that the instance is guarenteed to be in a Completed state. This guarentee allows
    //          for reliable Status() polling scenarios.
    //
    NTSTATUS
    Status();

    // Send a OnCancel() "signal" to the derivation implementation
    //
    // Only useful while in a non-completing state - ignored in all other states.
    // This can cause an outstanding AcquireLock() to be completed with !LockAcquiredCallback.IsAcquired.
    //
    // Returns: TRUE if the cancel request was accepted by the internal state machine.
    //
    // Can be called from any thread
    //
    BOOLEAN
    Cancel();

    // Reset the instance for another operation
    //
    // Only valid while in a completed state - failfast in all other states.
    //
    // Can be called from any thread
    //
    VOID
    Reuse();

    typedef KDelegate<VOID(
        KAsyncContextBase* const,           // Parent; can be nullptr
        KAsyncContextBase&                  // CompletingSubOp
        )> CompletionCallback;

    // Return the (optional) KAsyncGlobalContext associated with the current KAsyncContextBase instance.
    //
    // Returns either the parent's KAsyncGlobalContext or the override passed to Start()
    //
    // NOTE: will return an empty SPtr once completion has occured.
    //
    KAsyncGlobalContext::SPtr&
    GetGlobalContext();

    // Override the KAsyncGlobalContext associated with a KAsyncContextBase instance
    VOID
    SetGlobalContext(KAsyncGlobalContext::SPtr GlobalContext);

    // Return the activity id from the KAsyncGlobalContext if there is one.
    KActivityId
    GetActivityId();

    // Determine if calling thread is executing within the current KAsyncContextBase apartment
    BOOLEAN
    IsInApartment();

    // Generate a callback in the current KAsyncContextBase apartment.
    NTSTATUS
    CallbackAsyncContext(__in ULONG AllocationTag, __in CompletionCallback Completion);


    #if KTL_USER_MODE
    //* Advanced threading behavior control API

    // Control which Thread Pool the current async uses to schedule worker threads for itself.
    //
    //      By default an async obtains its thread pool (ktl-dedicated or system) from the KtlSystem instance with
    //      which it resides - see KtlSystem::SetDefaultSystemThreadPoolUsage(). Typically, if needed, the default
    //      thread pool is set just before calling a derivation's Start*() method or by the derivation during Reuse().
    //
    //      The KtlSystem::SetDefaultSystemThreadPoolUsage() value is captured during ctor of a KAsyncContextBase.
    //
    //  NOTE: Only valid while in a Initialized state - failfast in all other states.
    //
    VOID
    SetDefaultSystemThreadPoolUsage(BOOLEAN OnOrOff);

    BOOLEAN
    GetDefaultSystemThreadPoolUsage();

    // Control the detachment behavior of the completion callback thread
    //
    // KTL defaults to the assumption completion callbacks (see Start()) will do very little work and not block the thread.
    // This default favors performance. There are cases where the behavior of the callback is not known or the user of an async
    // which supplies the callback knows it will keep the thread for its extended use. By using this API the default behavior
    // can be changed by the derivation or user of an async. When set TRUE the callback (system TP only) thread is detached
    // from the threadpool and the async (like ReleaseApartment()) just before the callback.
    //
    //  Default value: FALSE; set FALSE on each Reuse().
    //
    //  NOTES:
    //      - This feature only works for non-parent bound cycles (Start-Complete-Reuse). Parent provided completion callbacks
    //        can use ReleaseApartment() to free up the callback thread for abritrary use.
    //      - This option can only be used with the system thread pool or a fail fast will occur.
    //        See: KThreadPool::DetachCurrentThread()
    //      - Can only be set while in a Initialized or Operating states - failfast in all other states
    //
    VOID
    SetDetachThreadOnCompletion(BOOLEAN OnOrOff);

    BOOLEAN
    GetDetachThreadOnCompletion();
    #else
    //
    // Controls how the async is dispatched. Although DPC dispatch flag may change dynamically, the value for it
    // is snapped when the async starts and is sticky until the async
    // completes.  Note there are additional restrictions on asyncs that are set to dispatch on a DPC thread.
    // These are the same restrictions that apply to any code running
    // at DPC. Some are described below:
    //
    // Asyncs that run on a DPC thread run at IRQL = DISPATCH and this need to follow the following guidelines:
    //     * All code and data must be non-paged
    //     * All apis called must be safe for running at DISPATCH
    //     * Other restrictions as described in the DDK
    //
    // Asyncs that are DPCDispatch may have children that are DPCDispatch but not children that are not DPCDispatch.
    // Asyncs that are not DPCDispatch may not have parents that are DPCDispatch.
    //
    // Even for DPCDispatch asyncs, not all async operations are dispatched on a DPC thread:
    //
    //     OnReuse() - always invoked at Reuse() callers IRQL.  This is
    //                 ok as the contract is for the async to be ready to go once Reuse() returns.
    //     OnStart() - always invoked at DPC
    //     OnComplete() - always invoked at DPC
    //     OnCancel() - always invoked at DPC
    //     Completion callback - always invoked at DPC. Note implications for code that starts a DPCDispatch async
    //
    // Parameters
    //    SetDPCDispatch  - TRUE if the async should be marked as DPCDispatch when next started
    //
    NTSTATUS
    SetDispatchOnDPCThread(
        __in BOOLEAN SetDPCDispatch
        );
    #endif


protected:
    // Derivation support API

    typedef KDelegate<VOID(
        BOOLEAN,                            // IsAcquired
        KAsyncLock&                         // LockAttempted
        )> LockAcquiredCallback;

    typedef KDelegate<VOID(
        void*                               // Context
        )> CompletingCallback;

    // Start the state machine for a new operation
    //
    // Only useful while in an initialized state - failfast in all other states.
    // This will result in an eventual OnStart() call to the derivation.
    //
    // Parameters:
    //      ParentAsyncContext    - A pointer to an optional parent KAsyncContextBase instance that is to
    //                              receive and dispatch completions thru. The value of this parameter
    //                              may be nullptr to disable the parent relationship. A referenced parent
    //                              will not complete as long as any reference child (aka SubOp) instances
    //                              are not completed.
    //
    //      CallbackPtr           - Optional completion callback. This function will be called when the instance enters
    //                              the completed state. NOTE: even if CallbackAsyncContext is empty, this callback will
    //                              still be called.
    //
    //      GlobalContextOverride - Optional pointer to a KAsyncGlobalContext derivation for the current operation. If one
    //                              is not supplied and their is a Parent, that parent's global context pointer will be
    //                              used. Access to the current operation's KAsyncGlobalContext is made available to the
    //                              derivation via GetGlobalContext - see below.
    //
    // Can be called from any thread. The object is Addref()'d to support an independent instance model. This self ref-count is
    // released immediately after parent (or independent) completion callback is called.
    //
    VOID
    Start(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt CompletionCallback CallbackPtr,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr);

    // Signal current operation completion
    //
    // Only useful while in an operating state.
    // This call will result in an eventual call to *CallbackPtr once all sub-operations referencing this instance are complete.
    // Also if a AcquireLock() is outstanding (not observed by the derivation), it will be canceled and if needed Signaled
    // automatically.
    //
    // Parameters:
    //      Status - resulting status - stored in _Status
    //
    // Returns:
    //      TRUE  - Call caused completion
    //      FALSE - Already completing - call ignored
    //
    // Can be called from any thread
    //
    BOOLEAN
    Complete(__in NTSTATUS Status);

    // Additional Parameters:
    //      Callback - called iif a TRUE would be returned
    //      CallbackContext - value pass to Callback if called
    //
    //      NOTE: When Callback is invoked, an internal spinlock is being held within the Complete() logic - use at your own risk
    //
    BOOLEAN
    Complete(__in_opt CompletingCallback Callback, __in_opt void* CallbackContext, __in NTSTATUS Status);

    // Attempt to acquire ownership of an KAsyncLock instance
    //
    // Only useful while in an operational OR completing state - failfast in all other states.
    // This will result in an eventual callback to the derivation indicating if the lock was acquired. Several conditions can cause
    // completion of an acquire attempt indicating that the lock was not acquired: Cancel(), Complete(), or any sub-operation
    // completion. Only one AcquireLock() attempt may be outstanding at a time.
    //
    // Parameters:
    //      LockRef          - Reference to lock to be acquired.
    //      AcquiredCallback     - completion callback - callback is only called from a "safe thread"
    //
    // Can be called from any thread BUT from within the apartment of the current KAsyncContextBase instance.
    //
    VOID
    AcquireLock(__in KAsyncLock& LockRef, __in LockAcquiredCallback AcquiredCallback);

    // Release ownership of an KAsyncLock instance
    //
    // Parameters:
    //      LockRef          - Reference to lock to be released.
    //
    // Can be called from any thread
    //
    VOID
    ReleaseLock(__in KAsyncLock& LockRef);

    // Test if this KAsyncContextBase instance is the owner of a KAsyncLock instance
    //
    // Parameters:
    //      LockRef          - Reference to lock to be tested.
    //
    // Returns:
    //      TRUE  - This KAsyncContextBase instance owns the specified lock.
    //      FALSE - This KAsyncContextBase instance does not own the specified lock.
    //
    // Can be called from any thread
    //
    BOOLEAN
    IsLockOwner(__in KAsyncLock& LockRef);

    // Try to increment the outstanding Activity Count for the instance
    //
    // If this instance is in an operational, completing, or starting state, the desired Activity Count will be acquired and this method returns TRUE.
    // In all other cases, this method returns FALSE.
    //
    // Parameters:
    //      NumberToAcquire     - Value to increment outstanding Activity Count by
    //
    // Can be called from any thread
    //
    BOOLEAN
    TryAcquireActivities(ULONG NumberToAcquire = 1);

    // Increment the outstanding Activity Count for the instance
    //
    // Only useful while in an operational, completing, or starting state - failfast in all other states.
    //
    // Parameters:
    //      NumberToAcquire     - Value to increment outstanding Activity Count by
    //
    // Can be called from any thread
    //
    VOID
    AcquireActivities(ULONG NumberToAcquire = 1);

    // Decrement the outstanding Activity Count for the instance
    //
    // Only useful while in an operational, completing, or starting state - failfast in all other states.
    //
    // Parameters:
    //      NumberToRelease     - Value to decrement outstanding Activity Count by
    //
    // Returns:
    //      TRUE                - If the outstanding Activity Count reached zero during this operation
    //
    // Can be called from any thread
    //
    BOOLEAN
    ReleaseActivities(ULONG NumberToRelease = 1);

    // Return the current value of the outstanding Activity Count for the instance
    ULONG
    QueryActivityCount();

    #if KTL_USER_MODE
    // Release the current KAsync's apartment.
    //
    //      After this call returns, the calling thread is no longer executing within the async's
    //      single threaded apartment. This implies a logic fork where the another thread may be
    //      executing in parallel to the current thread. When used in combination with the ability
    //      to use the System thread pool - see SetDefaultSystemThreadPoolUsage() - aribitrary actions
    //      such as suspending of the thread on sync objects.
    //
    //      Releasing an async's appartment implies detaching the caller's thread from the thread pool
    //      if using the system thread pool - See: KThreadPool::DetachCurrentThread().
    //
    //  NOTE: It is only valid to call this method while the current async is in the Operating or Completing
    //        states (e.g. OnStart(), OnCancel(), LockAcquiredCallback, or sub-op completion) or
    //        a FAILFAST will occur.
    //
    VOID
    ReleaseApartment();
    #endif

protected:
    // Derivation extension API: These methods and any callbacks provided to sub-operations and AcquireLock() are dispatched
    // one-at-a-time. Meaning that the derivation implementation does not have to be concerned with race conditions.

    // Derivation should start its current operational activity (eg sub-ops, low-level system service usage, acquiring locks,...).
    //
    // Default Behavior: Complete(STATUS_SUCCESS) is called.
    //
    virtual VOID
    OnStart();

    // Derivation should stop and complete its current operational activity at fast as possible.
    //
    // Default Behavior: None.
    //
    virtual VOID
    OnCancel();

    // Reuse() has been called.
    //
    // The derivation should release all internal resources related to its last operation and prepare
    // for another operation. The instance should be in the same state upon return from this call as
    // if a freshly constructed instance.
    //
    // Default Behavior: none.
    //
    virtual VOID
    OnReuse();

    // Inform the derivation that the current instance has entered its Completed state.
    //
    // The derivation can use this optional method to do any final cleanup work related to the current
    // operation cycle. This method is called just before a Start()-supplied completion callback
    // is called. This means that the implementation of this method has full access to the current
    // instance's state.
    //
    // NOTE: This method is called in the Apartment of the parent if one was supplied and the
    //       current instance. In either case it is safe to access the current instance's state.
    //
    virtual VOID
    OnCompleted();

private:
    //** Friend Derivation Class Extension API
    //   Only these special friend classes should override these private virtual methods.
    //   They are not for general use!!!
    friend class KAsyncLockHandleImp;
    friend class KAsyncLockContext;
    friend class KAsyncContextTestDerivation;
    friend class KAsyncContextInterceptor;

    virtual VOID
    FailFast();

    virtual VOID
    HandleOutstandingLocksHeldAtComplete();

    virtual VOID
    PrivateOnFinalize();

private:
    friend class KAsyncLock;
    using KObject<KAsyncContextBase>::Status;           // Trap calls to KObject::Status() into KAsyncContextBase::Status()
    using KObject<KAsyncContextBase>::SetStatus;        // Disallow derivation use of SetStatus() - must use Complete()

    // The KAsyncContextBase state machine defines the following event types:
    enum EventType : UCHAR
    {
        NoEvent,
        StartEvent,                 // Start() is called
        CompleteEvent,              // Complete() is called
        CancelEvent,                // Cancel() is called
        WaitCompleteEvent,          // Lock was acquired - because the derivation called AcquireLock()
        WaitAlertEvent,             // Lock acquire (wait) interrupted - because a Schedule*() method was called
        SubOpCompleteEvent,         // A sub-operation (another KAsyncContextBase instance) is completing
        CompletionDoneEvent         // This instance is internally complete - and w/Parent KAsyncContextBase
    };

    // At any moment the instance level dispatcher is in one of the following states:
    enum DispatcherState : UCHAR
    {
        WaitingOnAcquireLock,       // The instance is queued waiting for ownership of one (only) KAsyncLock
        Runnable,                   // The instance is waiting for thread assignment by the ThreadPool
        Running,                    // A thread is actively processing events (one-at-a-time) for the instance
        Idle                        // No work is pending; being processed; lock being acquired, and no thread is assigned
    };

    // holds pending one-shot event indications:
    struct EventFlags
    {
    public:
        BOOLEAN _StartPending           : 1;
        BOOLEAN _CompletePending        : 1;
        BOOLEAN _CancelPending          : 1;
        BOOLEAN _WaitCompletePending    : 1;
        BOOLEAN _AlertCompletePending   : 1;
        BOOLEAN _CompletionDonePending  : 1;

        inline BOOLEAN
        IsClear();

        inline BOOLEAN
        IsWorkPending()
        {
            return _StartPending ||
                   _CompletePending ||
                   _CancelPending ||
                   _WaitCompletePending ||
                   _AlertCompletePending ||
                   _CompletionDonePending;
        }

    protected:
        EventFlags();
    };

    // All pending work (except for queued sub-op completions) and DispatcherState are combined for size considerations.
    // The other data representing pending work for an instance is _CompletionQueue.
    struct ExecutionState : public EventFlags
    {
    public:
        ExecutionState();

        inline BOOLEAN
        IsClear();

        DispatcherState _DispatcherState : 8;
        BOOLEAN         _AcquireLockRequested : 1;
        #if KTL_USER_MODE
            BOOLEAN         _DetachThreadOnCompletion : 1;              // Thread will be detached before completion callback
            BOOLEAN         _UsingSystemThreadPool : 1;                 // Captures the default thread pool for this instance
            BOOLEAN         _KtlUsingSystemThreadPool : 1;              // Captures the default thread pool for ThisKtlSystem
        #else
            BOOLEAN         _IsDPCDispatch : 1;                         // When async is started dispatch async on a DPC thread
            BOOLEAN         _IsCurrentlyDPCDispatch : 1;                // If async currently running then dispatch async on a DPC thread
        #endif
    };

    // Data structure types for the overloaded _WaitListEntrySpace storage space
    class WorkItem : public KThreadPool::WorkItem
    {
    public:
        VOID Execute() override;
    };

    // Routines to schedule various events from ANY thread - does not cause direct dispatch. Intutative view: Each of these
    // methods "sends" the input parameters as a logical "message" (event) to the internal KAsyncContextBase instance level
    // state machine for processing on a safe thread - thread scheduling is done as needed. Concrete View: Most of these
    // scheduling primitives only support one event (message) per type to be outstanding. The exceptions to this rule are
    // Cancel and Sub-Operation completions. It is expected that Cancels can be done at any time and the state machine is very
    // supportive of this by ignoring dup cancels. Sub-Operations are kept on a list and thus don't have this "one-shot"
    // limitation.

    // BUG, richhas, xxxxx, Consider: if running at PASSIVE_LEVEL - hyjack calling thread for dispatch; else schedule at
    //                                hiPri if thread not scheduled already for the current context.
    VOID
    ScheduleStart(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in CompletionCallback CallbackPtr,
        __in_opt KAsyncGlobalContext* const GlobalContext);

    BOOLEAN
    ScheduleCancel();

    // BUG, richhas, xxxxx, Consider: If not running at PASSIVE_LEVEL, hyjack calling thread for dispatch; else
    //                                schedule dispatcher on existing if thread not scheduled already for the
    //                                current context.
    //
    // TRUE returned if completion actually scheduled
    BOOLEAN
    ScheduleComplete(
        __in NTSTATUS CompletionStatus,
        __in_opt CompletingCallback Callback,
        __in void* CallbackContext);

    BOOLEAN
    ScheduleCompletionDone();                               // TRUE - if completion executed (not just scheduled)

    VOID
    ScheduleLockAcquireCompletion();

    VOID
    ScheduleSubOpCompletion(__in KAsyncContextBase& CompletedSubOpContext);

    inline BOOLEAN
    UnsafeEnsureThread();

    inline KThreadPool& GetThreadPool();

    VOID
    ScheduleThreadPoolThread();

    // Instance Level Dispatcher and State Machine.
    //
    // The Instance Level State Machine is made of a set of state specific dispatchers; each with the following signature:
    typedef BOOLEAN (KAsyncContextBase::* StateSpecificDispatcher)(__in BOOLEAN Continued); // returns true to continue
                                                                                            // dispatching - allows for
                                                                                            // change-state-and-continue
                                                                                            // semantics

    // Concretely there are five formal states with fixed set of allowed event types for each:
    //State: Event-> StartEvent CompleteEvent CancelEvent WaitCompleteEvent SubOpCompleteEvent CompletionDoneEvent WaitAlertEvent
    //  Initalized       -  y          Y1           Y1              N                   N                   N            N
    //  Operating        -  N          y            Y               Y                   Y                   N            Y
    //  Completing       -  N          Y1           Y1              Y                   Y                   N            Y
    //  CompletionPending-  N          Y1           Y1              N                   N                   y            N
    //  Completed*       -  N          Y1           Y1              N                   N                   N            N
    //
    //  y  - Event is accepted and may/will cause change of state to the next state
    //  Y  - Event is accpeted
    //  N  - Event causes system fault
    //  1  - Event is dropped - not observed by the derivation
    //  2  - Event is dropped after releasing the lock (Signal) - not observed by the derivation
    //  *  - Only way to change to Initalized state is via Reuse() call

    // Entry point for ThreadPool dispatch
    VOID
    Dispatcher();

    // Dispatchers for each possible state.
    BOOLEAN
    InitializedStateDispatcher(__in BOOLEAN Continued);

    BOOLEAN
    OperatingStateDispatcher(__in BOOLEAN Continued);

    BOOLEAN
    CompletingStateDispatcher(__in BOOLEAN Continued);

    BOOLEAN
    CompletionPendingStateDispatcher(__in BOOLEAN Continued);

    BOOLEAN
    CompletedStateDispatcher(__in BOOLEAN Continued);

    EventType
    UnsafeGetNextEvent();

    inline VOID
    SubOpStarted();

    BOOLEAN
    UnsafeCancelPendingAcquireLock();

    inline BOOLEAN
    UnsafeIsPendingLockAcquire(__in KAsyncLock& ForLock);

    inline BOOLEAN
    IsInitializedState();

    inline BOOLEAN
    IsOperatingState();

    inline BOOLEAN
    IsStarting();

    inline BOOLEAN
    IsCompletingState();

    inline BOOLEAN
    IsCompletionPendingState();

    inline BOOLEAN
    IsCompletedState();

    inline BOOLEAN
    IsReusable();

    inline BOOLEAN
    IsWorkPending();

    inline static KAsyncContextBase*
    KAsyncContextBasePtrFromWaitListEntry(__in LinkListEntry* WaitListEntryPtr);

    inline static KAsyncContextBase*
    KAsyncContextBasePtrFromWorkItemEntry(__in WorkItem* WorkItemEntryPtr);

    inline BOOLEAN
    IsAcquiringLock();

    inline LinkListEntry&
    WaitListEntry();

    inline WorkItem&
    WorkItemEntry();

    inline VOID
    InitializeWaitListEntry();

    inline VOID
    FreeWaitListEntry();

    inline VOID
    InitializeWorkItemEntry();

    inline VOID
    FreeWorkItemEntry();

    #if ! KTL_USER_MODE
    static VOID
    DPCHandler(
        __in PKDPC DPC,
        __in_opt PVOID Context,
        __in_opt PVOID SystemArgument1,
        __in_opt PVOID SystemArgument2
        );

    #endif
private:
    KSpinLock                   _ThisLock;
    ExecutionState              _ExecutionState;
    LinkListHeader              _CompletionQueue;   // BUG, richhas, xxxxx, consider only fwd link - saves 8 bytes
    ULONG                       _PendingWorkCount;  // Includes all sub-ops that are not complete;
                                                    // a pending lock acquire (if outstanding); and additionally
                                                    // includes count events via AcquireActivity()/ReleaseActivities()
    LONG volatile               _LocksHeld;         // Count of KAsyncLocks owned by this instance
    KThread::Id volatile        _CurrentDispatchThreadId;

    #if KTL_USER_MODE
        // State to support ReleaseApartment() behavior
        class ApartmentState;
        friend class ApartmentState;
        ApartmentState* volatile    _ApartmentState;
    #endif

    #if !KTL_USER_MODE
        KUniquePtr<KDPC> _KDPC;
    #endif

    // Current State:
    StateSpecificDispatcher     _CurrentStateDispatcher;

    // The use of the _WaitListEntrySpace storage is overloaded:
    //  _ExecutionState._DispatcherState == WaitingOnAcquireLock        -- ListListEntry owned by the wait object (_WaitObject) - queued
    //                                                                     waiting for signal
    //                                   == Runnable                    -- WorkItem owned by the ThreadPool - queued as work item
    //                                                                     awaiting a worker thread
    //  *_CurrentStateDispatcher         == CompletingStateDispatcher   -- ListListEntry owned by *_CallbackAsyncContext - instance
    //                                                                     awaiting completion dispatch - see _CompletionQueue
    //
    static const int            _WaitListEntrySpacePVoidCount = 3;
    PVOID                       _WaitListEntrySpace[_WaitListEntrySpacePVoidCount];
    static const int            _SizeofWaitListEntry = _WaitListEntrySpacePVoidCount * sizeof(PVOID);

    static_assert(sizeof(LinkListEntry) <= _SizeofWaitListEntry, "_WaitListEntrySpace too small");
    static_assert(sizeof(WorkItem) <= _SizeofWaitListEntry, "_WaitListEntrySpace too small");

    // WaitingOnAcquireLock related state
    KAsyncLock*                 _LockPtr;
    LockAcquiredCallback        _AcquiredLockCallback;

    // Operation completion callback related state (see Start())
    KAsyncContextBase::SPtr     _ParentAsyncContextPtr;
    CompletionCallback          _CallbackPtr;

    // Global Context support related state
    KAsyncGlobalContext::SPtr   _GlobalContext;
};

// inline method implementations
inline BOOLEAN
KAsyncContextBase::EventFlags::IsClear()
{
    return !(_StartPending
        || _CompletePending
        || _CancelPending
        || _WaitCompletePending
        || _AlertCompletePending
        || _CompletionDonePending);
}

inline BOOLEAN
KAsyncContextBase::ExecutionState::IsClear()
{
    return !_AcquireLockRequested && EventFlags::IsClear();
}

inline BOOLEAN
KAsyncContextBase::IsWorkPending()
{
    return _ExecutionState.IsWorkPending() || !_CompletionQueue.IsEmpty();
}

inline BOOLEAN
KAsyncContextBase::UnsafeEnsureThread()
{
    // KAssert(_ThisLock.IsHeld());
    BOOLEAN threadNeeded = (_ExecutionState._DispatcherState != Runnable)
                        && (_ExecutionState._DispatcherState != Running);

    if (threadNeeded)
    {
        if ((_ExecutionState._DispatcherState == WaitingOnAcquireLock) && UnsafeCancelPendingAcquireLock())
        {
            KInvariant(!_ExecutionState._AlertCompletePending);
            _ExecutionState._AlertCompletePending = TRUE;
        }

        KInvariant(!WaitListEntry().IsLinked());    // Must not be linked
        _ExecutionState._DispatcherState = Runnable;
    }

    return threadNeeded;
}

// SubOps report in here as they Start()
inline VOID
KAsyncContextBase::SubOpStarted()
{
    _ThisLock.Acquire();
    {
        // Sub-ops can be started in any of the following states:
        KInvariant(IsStarting() || IsOperatingState() || IsCompletingState());
        _PendingWorkCount++;
    }
    _ThisLock.Release();
}

// NOTE: This is "called" (inlined) only by KAsyncLock::CancelAcquire to which is ONLY called by
//       UnsafeCancelPendingAcquireLock(). This is done for the sake of modularity and lock order.
inline BOOLEAN
KAsyncContextBase::UnsafeIsPendingLockAcquire(__in KAsyncLock& ForLock)
{
    // KAssert(_ThisLock.IsHeld());
    return IsAcquiringLock() && !_ExecutionState._WaitCompletePending && (_LockPtr == &ForLock);
}

inline BOOLEAN
KAsyncContextBase::IsInitializedState()
{
    return _CurrentStateDispatcher == &KAsyncContextBase::InitializedStateDispatcher;
}

inline BOOLEAN
KAsyncContextBase::IsStarting()
{
    return (IsInitializedState() && _ExecutionState._StartPending);
}

inline BOOLEAN
KAsyncContextBase::IsOperatingState()
{
    return _CurrentStateDispatcher == &KAsyncContextBase::OperatingStateDispatcher;
}

inline BOOLEAN
KAsyncContextBase::IsCompletingState()
{
    return _CurrentStateDispatcher == &KAsyncContextBase::CompletingStateDispatcher;
}

inline BOOLEAN
KAsyncContextBase::IsCompletionPendingState()
{
    return _CurrentStateDispatcher == &KAsyncContextBase::CompletionPendingStateDispatcher;
}

inline BOOLEAN
KAsyncContextBase::IsCompletedState()
{
    return _CurrentStateDispatcher == &KAsyncContextBase::CompletedStateDispatcher;
}

inline BOOLEAN
KAsyncContextBase::IsReusable()
{
    return IsInitializedState() || IsCompletedState();
}

inline KAsyncContextBase*
KAsyncContextBase::KAsyncContextBasePtrFromWaitListEntry(__in LinkListEntry* WaitListEntryPtr)
{
    static const SIZE_T offsetOfWaitListEntry = (SIZE_T)&(((KAsyncContextBase*)nullptr)->_WaitListEntrySpace);
    return (KAsyncContextBase*)(((UCHAR*)WaitListEntryPtr) - offsetOfWaitListEntry);
}

inline KAsyncContextBase*
KAsyncContextBase::KAsyncContextBasePtrFromWorkItemEntry(__in WorkItem* WorkItemEntryPtr)
{
    static const SIZE_T offsetOfWorkItemEntry = (SIZE_T)&(((KAsyncContextBase*)nullptr)->_WaitListEntrySpace);
    return (KAsyncContextBase*)(((UCHAR*)WorkItemEntryPtr) - offsetOfWorkItemEntry);
}

inline BOOLEAN
KAsyncContextBase::IsAcquiringLock()
{
    return _LockPtr != nullptr;
}

inline LinkListEntry&
KAsyncContextBase::WaitListEntry()
{
    return *reinterpret_cast<LinkListEntry*>(&_WaitListEntrySpace[0]);
}

inline KAsyncContextBase::WorkItem&
KAsyncContextBase::WorkItemEntry()
{
    return (*reinterpret_cast<KAsyncContextBase::WorkItem*>(&_WaitListEntrySpace[0]));
}

inline VOID
KAsyncContextBase::InitializeWaitListEntry()
{
    new(&_WaitListEntrySpace[0]) LinkListEntry();
}

inline VOID
KAsyncContextBase::FreeWaitListEntry()
{
    WaitListEntry().~LinkListEntry();
}

inline VOID
KAsyncContextBase::InitializeWorkItemEntry()
{
    new(&_WaitListEntrySpace[0]) WorkItem();
}

inline VOID
KAsyncContextBase::FreeWorkItemEntry()
{
    WorkItemEntry().~WorkItem();
}


//** Test Support base class that intercept usage of other KAsyncContext instances
//
//   This class is used by test arrangments that need to intercept KAsync instances
//   that already exist in a transparent fashion. In its raw fashion (without a derivation),
//   instances of this class injects itself between an application and an existing KAsync
//   derivation. It does this by re-directing the v-table of the intercepted instance such
//   when the On*() methods are dispatched by the KAsyncContextBase control is trapped into
//   this class's implementation. This disables all derivation code of the intercepted instance.
//   This interception is initiated via EnableIntercept() and disabled via DisableIntercept().
//   DisableIntercept() restores the intercepted instance's v-table to its original state.
//
//   While intercepted, the intercepted instance's On*() methods are redirected to this class's
//   corresponding On*Intercepted() methods which may be overridden by KAsyncContextInterceptor
//   derivations - by default these methods do nothing.
//
//   NOTE: While KAsyncContextInterceptor is a derivation of KAsyncContextBase, it it not intended
//         to be a functional KAsync instance in its own right - this may be changed in the future.
//
class KAsyncContextInterceptor : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncContextInterceptor);

public:
    //* KAsyncContextInterceptor factory
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KSharedPtr<KAsyncContextInterceptor>& Context);

    //* Start interception of a subject KAsyncContextBase instance derivation behavior.
    //
    //  Parameters: OnAsync - the KAsync instance to be intercepted. This instance must be in
    //                        an initialized state in order to intercept. All other state will
    //                        cause a fail fast.
    //
    //  On return the v-table for the intercepted instance is redirected to this instance. NOTE:
    //  calling any virtual methods on the intercepted instance directly while it is intercepted
    //  is not supported and will cause bad things to happen.
    //
    VOID EnableIntercept(KAsyncContextBase& OnAsync);

    //* Disable interception of KAsyncContextBase instance derivation behavior.
    //
    //  NOTE: the intercepted instance can't be in a STATUS_PENDING state or a fail fast will occur.
    //
    VOID DisableIntercept();

    //* Return a reference to the current intercepted instance
    inline KAsyncContextBase&
    GetIntercepted()
    {
        KAsyncContextBase*  result = _InterceptedAsync.RawPtr();
        KFatal(result != nullptr);
        return *result;
    }

    VOID ForwardOnStart();
    VOID ForwardOnCancel();
    VOID ForwardOnReuse();
    VOID ForwardOnCompleted();

protected:
    virtual void OnStartIntercepted() {}
    virtual void OnCancelIntercepted() {}
    virtual void OnCompletedIntercepted() {}
    virtual void OnReuseIntercepted() {}

#if !defined(PLATFORM_UNIX)
    static const ULONG OnStartVtableOffset = 2;
#else
    static const ULONG OnStartVtableOffset = 3;
#endif
    typedef void(*OnStartCallback)(PVOID);
    OnStartCallback _OnStartCallback;

#if !defined(PLATFORM_UNIX)
    static const ULONG OnCancelVtableOffset = 3;
#else
    static const ULONG OnCancelVtableOffset = 4;
#endif
    typedef void(*OnCancelCallback)(PVOID);
    OnCancelCallback _OnCancelCallback;

#if !defined(PLATFORM_UNIX)
    static const ULONG OnReuseVtableOffset = 4;
#else
    static const ULONG OnReuseVtableOffset = 5;
#endif
    typedef void(*OnReuseCallback)(PVOID);
    OnReuseCallback _OnReuseCallback;

#if !defined(PLATFORM_UNIX)
    static const ULONG OnCompletedVtableOffset = 5;
#else
    static const ULONG OnCompletedVtableOffset = 6;
#endif
    typedef void(*OnCompletedCallback)(PVOID);
    OnCompletedCallback _OnCompletedCallback;

private:
    using KAsyncContextBase::Start;
    using KAsyncContextBase::Cancel;
    using KAsyncContextBase::Complete;
    using KAsyncContextBase::OnStart;
    using KAsyncContextBase::OnCancel;
    using KAsyncContextBase::OnCompleted;
    using KAsyncContextBase::OnReuse;

    VOID OnStart();
    VOID OnCancel();
    VOID OnCompleted();
    VOID OnReuse();
    VOID HandleOutstandingLocksHeldAtComplete();
    VOID PrivateOnFinalize();

private:

    static KGlobalSpinLockStorage      gs_GlobalLock;

    static KNodeList<KAsyncContextInterceptor>*
    UnsafeGetKAsyncContextInterceptors();

    static KAsyncContextInterceptor*
    FindInterceptor(KAsyncContextBase& ForIntercepted);

private:
    KListEntry              _InterceptorsListEntry;
    KAsyncContextBase::SPtr _InterceptedAsync;
    void*                   _SavedInterceptedVTablePtr;
};

//
// Use K_ASYNC_SUCCESS() macro to test if invoking an async operation is successful.
// It is logically identical to the NT_SUCCESS() macro but adds extra check.
// It should be used in place of NT_SUCCESS(),
//
// These are the rules for implementing an async API:
//  (1) An async API can return any error status code. If so, the implementation must NOT invoke the completion callback.
//  (2) Otherwise, the API MUST return STATUS_PENDING and guarantee to invoke the completion callback in all cases.
//
// With these rules, a successful async operation can only be completed through a completion callback.
// The caller of an async API can always expect the callback if K_ASYNC_SUCCESS(Status) is TRUE.
// There is only one code path to handle a success case.
//
// Caveat:
// The parameter to this macro should be an NTSTATUS variable. Avoid using a function call that returns an NTSTATUS
// as the macro parameter.
//

#define K_ASYNC_SUCCESS(Status) (NT_SUCCESS(Status) ? KFatal(Status == STATUS_PENDING), TRUE : FALSE)

// KTL++    Support for C++ co-routine support (co_await and co_yield)
//
// Describtion:
//		The new version of KTL supporting C++ async co-routines is called KTL++. This description will
//		cover basic notions of this new KTL++ support. The fundemental goals of the new support are to
//		integrate the proven high speed and efficient KTL scheduling facilities used in the KAsyncContextBase
//		implementation with a set of waitable and awaitable contexts that are supported directly by modern
//		C++ compilers' "co_await", "co_yield", and "co_return" keywords.
//
//		To declare an async function:
//			<Continuance-Type> FunctionName(...) {}
//
//			where <Continuance-Type> := { Task | Awaitable<> | kasync::StartAwaiter | KCoService$Open/CloseAwaiter }
//
//		Task - This a fire and forget co_routine without KTL++ provide caller sync facilities
//		Awaitable<_RetT> -  This type of co_routine supports an co_await for any of the other type of "calling" co_routines.
//								WHERE _RetT := {void | anyType}
//		kasync::StartAwaiter -	This type of Continuance is a wrapper Awaiter for Start*Async() methods on KAsyncContextBase
//								derivations. It allows the starting and waiting for completion via co_await keyword. Such an
//								co_await will yield the completion NTSTATUS the kasync's last op. By introducing simpile Start*Async()
//								kasync::StartAwaiter<> methods on the _AsyncType KAsyncContextBase derivation - see KTimer.h for
//								example of both instance and static StartTimerAsync() patterns.
//								NOTE: There are many returning-of-results approaches. A few are:
//										- The caller keeps a SPtr and extracts results from the subject kasync after co_wait on it
//										- Output parameters are passed to the API and before returning NTSTATUS, those outputs populated
//                              NOTE: StartAwaiter is a KShared<> derivation. Use the static factory Create(). StartAwaiter instances
//                                    are the equiv of a co_routine frame but with a ref count controlled life time. It leaves open the
//                                    the reuse possibility.
//
//		<TODO: richhas: Add: KCoService$Open/CloseAwaiter docs
//
//		System Structure and Runtime Context:
//			An instance of a KTL runtime is represented by, and logically contained in, an instance of the KtlSystemBase class. Typically the class is
//			derived from to house application specific bootstrap and shutdown behaviors. Most of the heavy lifting wrt startup and shutdown is managed
//			by the the base class itself. Typically there is one derived instance of KAsyncServiceBase representing the entire application runtime
//			paired with the application startup/shutdown behaviors in the application derivation of KtlSystemBase. This singular application KService is
//			Opened and Closed by application logic; so typically the application logic will tell the KtlSystem to start and shutdown. In the case of shutdown
//			once all application memory is released from all heaps the shutdown can complete and return to the application KSystemBase derived code.
//			This approach demands that all dynamic memory be allocated on the heaps of the subject KtlSystem and that all async OS operations be represent
//			by a corresponding KAsync instances. It is the job of the application's main service logic to trigger closure in all sub services and those
//			in-turn canceling as needed KAsync's wait for OS async activity and such.
//
//

#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX) && defined (_M_X64) && (_MSC_FULL_VER) && (_MSC_FULL_VER >= 190024215)
    // co_routines only valid on 64-bit ms compiler version 19 update 3 or above
    #if !defined(K_UseResumable)
        #define K_UseResumable
    #endif
#elif defined(PLATFORM_UNIX) && (__clang__) && (__clang_major__ >= 4)
    // For Linux, co_routines only valid on clang compiler version 4.* and above
    #if !defined(K_UseResumable)
        #define K_UseResumable
    #endif
#endif
#endif

#if defined(K_UseResumable)

#if !defined(PLATFORM_UNIX)
#if DBG
    // #define K_UseResumableDbgExt
#endif
#endif

#include <KResumable.h>
#include <kevent.h>
#include <algorithm>

using namespace std::experimental;

namespace ktl
{
    // Fire and forget async forked coroutine
    //
    // Function Usage:
    //
    //   /* As a static or global function */
    //   Task MyGlobalTask(...)
    //   {
    //   ...
    //   }
    //
    //   /* As an instance method in a KShared<> derivation
    //   class MyShared : public KShared<MyShared>
    //   {
    //       Task MySharedTask(...)
    //       {
    //           KCoShared$ApiEntry();   // Required to keep MyShared from dtor'ing until MySharedTask()
    //                                   // goes out of scope.
    //           ...
    //       }
    //   };
    //
    //   /* As an instance method on an active KAsyncContext */
    //   class MyAsync : public KAsyncContextBase...
    //   {
    //       Task MyAsyncTaskApi(...)
    //       {
    //           KCoAsync$TaskEntry();       // Required to keep MyAsync from completing until MyAsyncTaskApi()
    //                                       // goes out of scope.
    //           ...
    //       }
    //   };
    //
    struct Task final
    {
        struct promise_type
        {
            #if defined(PLATFORM_UNIX)
            void *operator new(__in size_t sz) noexcept
            #else
            template <typename... Args>
            void *operator new(__in size_t sz, std::nothrow_t, Args const&...) noexcept
            #endif
            {
#if defined(KCo$ForceTaskFrameAllocationFailure)
                {
                    if (::_g_ForceTaskFrameAllocationFailure)
                    {
                        return nullptr;
                    }
                }
#endif
#if defined(KCo$IncludeAllocationCounting)
                InterlockedIncrement((volatile LONG*) &_g_KCo$TotalObjects);
#endif
                return ::HeapAlloc(::GetProcessHeap(), 0, sz);
            }

            void operator delete(__in void* p) noexcept
            {
#if defined(KCo$IncludeAllocationCounting)
                KInvariant(InterlockedDecrement((volatile LONG*) &_g_KCo$TotalObjects) < MAXULONGLONG);
#endif
                ::HeapFree(::GetProcessHeap(), 0, p);
            }

            static Task get_return_object_on_allocation_failure() { return Task(true); }
            Task get_return_object() { return Task(false); }
            void return_void() {}
            auto initial_suspend() { return suspend_never{}; }
            auto final_suspend() { return suspend_never{}; }
#if defined(CLANG_5_0_1_PLUS)
            void unhandled_exception() noexcept
#else
            void set_exception(__in std::exception_ptr)
#endif
            {
                KInvariant(FALSE);
            }

            promise_type(promise_type const&) noexcept = delete;
            promise_type& operator=(promise_type const&) noexcept = delete;

        #if defined(K_UseResumableDbgExt)
            // Debug extension support
            promise_type()
            {
                K_LOCK_BLOCK(gs_GlobalLock.Lock())
                {
                    gs_List.InsertBottom(_ListEntry);
                }
            }

            ~promise_type()
            {
                K_LOCK_BLOCK(gs_GlobalLock.Lock())
                {
                    _ListEntry.Remove();
                }
            }

            LinkListEntry                   _ListEntry;
            static KGlobalSpinLockStorage   gs_GlobalLock;
            static LinkListHeader           gs_List;
        #else   
            promise_type() {}
        #endif
        };

        // Used to detect if a Task's co_routine frame was allocated
        bool IsTaskStarted() { return !_AllocationFailureTask; }

#if defined(K_UseResumableDbgExt)
        // Debug extension support
        using EnumCallback = void (*)(promise_type&);
        static void EnumActiveTasks(EnumCallback Callback);
#endif

    private:
        friend promise_type;
        bool        _AllocationFailureTask = false;

        Task(__in bool FailedAllocation) : _AllocationFailureTask(FailedAllocation) {}
    };

    // Alias for typeless exception state variable
    typedef std::exception_ptr ExceptionHandle;

    // Common state-machine state helper for Awaitable<> implementations
    #pragma warning(disable:4201)
    union AwaitableState
    {
    private:
        volatile unsigned __int32 _StateValue;
        struct
        {
            bool    _ResultReady : 1;   // Set if return_value() OR return_void OR set_exception() called
            bool    _Suspended : 1;     // Set iff _FinalAwaiter not set - await_suspend() suspended
            bool    _FinalHeld : 1;     // Set iif _Suspended not set - FinalAwaiter::await_suspend() will not resume, 
                                        //                              await_suspend() must
            bool    _CalloutOnFinalHeld : 1;    // Set if TryToScheduleCalloutOnFinalHeld() has been called and returned true
            bool    _AwaitSuspendActive : 1;    // Set iif await_suspend() has been called and is active. Once set first co_await
                                                // must complete before another can be invoked - racing co_await are illegal
            bool    _Resumed : 1;               // Set iif continuation calls to await_resume() after compiler statemachine is "done"
        } _StateBits;

    private:
        __forceinline bool TrySetBitsWhileNot(
            AwaitableState BitsToSet, 
            AwaitableState WhileNotSet, 
            AwaitableState* ResultOnTrue = nullptr)
        {
            AwaitableState          oldValue;
            AwaitableState          newValue;
            unsigned __int32        icxResult;
            static AwaitableState   invalidBits = AwaitableState::Make_None();

#if defined(KCo$UnitTest$AwaitableStateRaceTestCount)
            // Unit Test support
            bool                didSpin = false;
#endif

            // Safely set and test state bits
            do
            {
                oldValue._StateValue = _StateValue;

                KInvariant((oldValue._StateValue & invalidBits._StateValue) == 0);      // Memory corruption!

                if ((oldValue._StateValue & BitsToSet._StateValue) != 0)    // All set bits are one-shots
                {
                    // !Failfast!
                    AwaitableState      faultingBits;
                    faultingBits._StateValue = (oldValue._StateValue & BitsToSet._StateValue);

                    KInvariant(!faultingBits._StateBits._AwaitSuspendActive);   // Application fault: Awaitable<>: secondary co_awaits can't occur until a first one has
                                                                                // returned
                    KInvariant(!faultingBits._StateBits._Suspended);            // Application fault: Awaitable<>: secondary co_awaits can't occur until a first one has
                                                                                // returned

                    KInvariant(!faultingBits._StateBits._FinalHeld);            // System fault: Multiple completions detected from underpinnings
                    KInvariant(!faultingBits._StateBits._ResultReady);          // System fault: Multiple attempts to set completion state from underpinnings
                    KInvariant(!faultingBits._StateBits._CalloutOnFinalHeld);   // System fault: Multiple calls to TryToScheduleCalloutOnFinalHeld()
                    KInvariant(!faultingBits._StateBits._Resumed);              // System fault: Multiple calls to SetResumed()

                    KInvariant(FALSE);                                          // System Fault: Unknown bits - should never occur
                }

                if ((oldValue._StateValue & WhileNotSet._StateValue) != 0)
                {
                    // at least one of the "WhileNotSet" bits is set, abort set attempt
                    return false;
                }

                newValue._StateValue = oldValue._StateValue | BitsToSet._StateValue;
                icxResult = InterlockedCompareExchange((volatile LONG*) &_StateValue, newValue._StateValue, oldValue._StateValue);

#if defined(KCo$UnitTest$AwaitableStateRaceTestCount)
                // Unit Test support
                if (icxResult != oldValue._StateValue)
                {
                    InterlockedIncrement((volatile LONG*) &_g_K$UnitTest$AwaitableStateRaceTestCount);
                    didSpin = true;
                }
#endif

            } while (icxResult != oldValue._StateValue);

#if defined(KCo$UnitTest$AwaitableStateRaceTestCount)
            // Unit Test support to prove race occured, were detected, and resolved correctly
            if (didSpin)
            {
                if (BitsToSet.IsReady())
                    InterlockedIncrement((volatile LONG*) &_g_K$UnitTest$AwaitableStateRaceTestResultReadyCount);
                else if (BitsToSet.IsSuspended())
                    InterlockedIncrement((volatile LONG*) &_g_K$UnitTest$AwaitableStateRaceTestSuspendedCount);
                else if (BitsToSet.IsFinalHeld())
                    InterlockedIncrement((volatile LONG*) &_g_K$UnitTest$AwaitableStateRaceTestFinalHeldCount);
                else if (BitsToSet.IsCalloutOnFinalHeld()) {}
                else if (BitsToSet.IsAwaitSuspendActive()) {}
                else if (BitsToSet.IsResumed()) {}
                else
                    KInvariant(false);
            }
#endif

            if (ResultOnTrue != nullptr)
            {
                (*ResultOnTrue)._StateValue = newValue._StateValue;
            }
            return true;
        }

    public:
        __forceinline AwaitableState() : _StateValue(0) {};
        __forceinline AwaitableState(const AwaitableState& Other) : _StateValue(Other._StateValue) {};

        __forceinline void Reuse() { _StateValue = 0; }

        __forceinline void ResultReady()
        {
            // Indicate Ready: Result (void, value, or exception yielded)
            TrySetBitsWhileNot(Make_ResultReady(), Make_ResultReady());
        }

        __forceinline bool TryToSuspend()
        {
            // Set Suspended iif TryToFinalize() has not raced ahead
            return TrySetBitsWhileNot(Make_Suspended(), Make_FinalHeld());
        }

        __forceinline bool TryToFinalize(AwaitableState* ResultOnTrue = nullptr)
        {
            // Set _FinalHeld iif TryToSuspend() has not raced ahead
            return TrySetBitsWhileNot(Make_FinalHeld(), Make_Suspended(), ResultOnTrue);
        }

        __forceinline bool TryToScheduleCalloutOnFinalHeld()
        {
            // Set _CalloutOnFinalHeld iif TryToFinalize() has not raced ahead
            return TrySetBitsWhileNot(Make_CalloutOnFinalHeld(), Make_FinalHeld());
        }

        __forceinline void SetAwaitSuspendActive()
        {
            // Indicate suspend_await (co_await) is active and must be completed before another call to is is valid
            TrySetBitsWhileNot(Make_AwaitSuspendActive(), Make_AwaitSuspendActive());
        }

        __forceinline void SetResumed()
        {
            // Indicate that continuation occured async via resume()
            TrySetBitsWhileNot(Make_Resumed(), Make_None());
        }

        __forceinline bool IsUnusedBitsValid()
        { 
            static AwaitableState   invalidBits = AwaitableState::Make_None();
            return ((_StateValue & invalidBits._StateValue) == 0); 
        }

        __forceinline bool IsReady() { return _StateBits._ResultReady; }
        __forceinline bool IsSuspended() { return _StateBits._Suspended; }
        __forceinline bool IsFinalHeld() { return _StateBits._FinalHeld; }
        __forceinline bool IsCalloutOnFinalHeld() { return _StateBits._CalloutOnFinalHeld; }
        __forceinline bool IsAwaitSuspendActive() { return _StateBits._AwaitSuspendActive; }
        __forceinline bool IsResumed() { return _StateBits._Resumed; }
        __forceinline ULONGLONG GetValue() { return _StateValue; }

        __forceinline static AwaitableState Make_ResultReady() { AwaitableState x; x._StateBits._ResultReady = true; return x; }
        __forceinline static AwaitableState Make_Suspended() { AwaitableState x; x._StateBits._Suspended = true; return x; }
        __forceinline static AwaitableState Make_FinalHeld() { AwaitableState x; x._StateBits._FinalHeld = true; return x; }
        __forceinline static AwaitableState Make_CalloutOnFinalHeld() { AwaitableState x; x._StateBits._CalloutOnFinalHeld = true; return x; }
        __forceinline static AwaitableState Make_AwaitSuspendActive() { AwaitableState x; x._StateBits._AwaitSuspendActive = true; return x; }
        __forceinline static AwaitableState Make_Resumed() { AwaitableState x; x._StateBits._Resumed = true; return x; }

        __forceinline static AwaitableState Make_None()
        { 
            AwaitableState x;
            x._StateValue = (unsigned __int32)-1;
            x._StateBits._FinalHeld = false; 
            x._StateBits._Suspended = false;
            x._StateBits._ResultReady = false;
            x._StateBits._CalloutOnFinalHeld = false;
            x._StateBits._AwaitSuspendActive = false;
            x._StateBits._Resumed = false;
            return x;
        }

        __forceinline bool ResetCalloutOnFinalHeld()
        {
            AwaitableState          oldValue;
            AwaitableState          newValue;
            unsigned __int32        icxResult;
            const unsigned __int32  notCalloutOnFinalHeld = ~Make_CalloutOnFinalHeld()._StateValue;

            do
            {
                oldValue._StateValue = _StateValue;
                newValue._StateValue = oldValue._StateValue & notCalloutOnFinalHeld;
                icxResult = InterlockedCompareExchange((volatile LONG*) &_StateValue, newValue._StateValue, oldValue._StateValue);
            } while (icxResult != oldValue._StateValue);

            return oldValue.IsCalloutOnFinalHeld();
        }
    };
    #pragma warning(default:4201)

    // Special exceptions from underlying co_routine allocation plumbing
    struct FrameNotBoundException {};

    // Common base type of Awaitable<>::promise_type
    struct PromiseCommon
    {
    #if defined(K_UseResumableDbgExt)
        static KGlobalSpinLockStorage   gs_GlobalLock;
        static LinkListHeader           gs_List;
        LinkListEntry                   _ListEntry;
        void*                           _ReturnAddress;

        using EnumCallback = void(*)(PromiseCommon&);
        static void EnumActiveAwaitables(EnumCallback Callback);
    #endif

        using AwaitableCompletedCallback = void(*)(void* Ctx);

        AwaitableCompletedCallback  _CompletedCallback = nullptr;
        void*                       _CompletedCallbackCtx = nullptr;
        coroutine_handle<>          _awaiterHandle = nullptr;
        std::exception_ptr          _exception = nullptr;
        AwaitableState              _state;

#if defined(PLATFORM_UNIX)
        void *operator new(__in size_t sz) noexcept
#else
        template <typename... Args>
        void *operator new(__in size_t sz, std::nothrow_t, Args const&...) noexcept
#endif
        {
#if defined(KCo$ForceAwaitableTFrameAllocationFailure)
            if (::_g_ForceAwaitableTFrameAllocationFailure)
            {
                return nullptr;
            }
#endif
#if defined(KCo$IncludeAllocationCounting)
            InterlockedIncrement((volatile LONG*) &_g_KCo$TotalObjects);
#endif
            return ::HeapAlloc(::GetProcessHeap(), 0, sz);
        }

        void operator delete(__in void* p) noexcept
        {
#if defined(KCo$IncludeAllocationCounting)
            KInvariant(InterlockedDecrement((volatile LONG*) &_g_KCo$TotalObjects) < MAXULONGLONG);
#endif
            ::HeapFree(::GetProcessHeap(), 0, p);
        }
        AwaitableState GetState() { return _state; }

        // Guarentee co_routines execute sync until first co_await is called
        auto initial_suspend() noexcept { return std::experimental::suspend_never{}; }

#if defined(CLANG_5_0_1_PLUS)
        void unhandled_exception() noexcept
        {
            _exception = std::current_exception();
#else
        void set_exception(__in std::exception_ptr Exception)
        {
            _exception = Exception;
#endif
            MemoryBarrier();

            // Safely mark as return value ready
            _state.ResultReady();
        }

    };

    #if defined(K_UseResumableDbgExt)
        static_assert(offsetof(PromiseCommon, _ListEntry) == 0, "_ListEntry must be at offset 0");
    #endif

    template <typename _RetT>
    struct Awaitable;

    template <typename _PromiseT>
    struct FinalAwaiter
    {
        bool await_ready() { return false; }
        void await_resume() {}

        static void await_suspend(__in coroutine_handle<_PromiseT> h)
        {
            KInvariant(h.promise()._state.IsReady());

            // Defer continuance thru await_suspend() if this completion path raced ahead
            AwaitableState      finalizedState;
            if (h.promise()._state.TryToFinalize(&finalizedState))
            {
                // NOTE: Once here "this" nor h can be considered stable because await_suspend() is allowed to 
                //       cause continuance on another thread.

                if (finalizedState.IsCalloutOnFinalHeld())
                {
                    // Callout scheduled; reset state and do the callout; h is stable
                    if (h.promise()._state.ResetCalloutOnFinalHeld())
                    {
                        h.promise()._CompletedCallback(h.promise()._CompletedCallbackCtx);
                    }
                }

                return;     // continuance will occur via await_suspend()
            }

            // await_suspend() raced ahead - we'll resume on this path
            KInvariant(!h.promise()._awaiterHandle.done());

#if defined(K_UseResumableDbgExt)
            // Untrack this awaiting promise
            h.promise()._ReturnAddress = nullptr;
            K_LOCK_BLOCK(PromiseCommon::gs_GlobalLock.Lock())
            {
                h.promise()._ListEntry.Remove();
            }
#endif

            h.promise()._awaiterHandle.resume();
        }
    };

    template <typename _RetT>
    struct Promise : public PromiseCommon
    {
        _RetT value() const
        {
            return _returnValue;
        }

        void return_value(__in const _RetT& Value)
        {
            _returnValue = Value;
            MemoryBarrier();

            // Safely mark as return value ready
            _state.ResultReady();
        }

        Awaitable<_RetT> get_return_object();
        // Called when allocation fails in operator new above
        static Awaitable<_RetT> get_return_object_on_allocation_failure();

        FinalAwaiter<Promise> final_suspend() noexcept { return FinalAwaiter<Promise>{}; }

        Promise() = default;
        Promise(Promise const&) noexcept = delete;
        Promise& operator=(Promise const&) noexcept = delete;
    private:
        _RetT _returnValue;
    };

    template <>
    struct Promise<void> : public PromiseCommon
    {
        using PromiseCommon::_CompletedCallback;

        void value() const
        {
        }

        void return_void()
        {
            // Safely mark as return void yielded (ready)
            _state.ResultReady();
        }

        Awaitable<void> get_return_object();
        // Called when allocation fails in operator new above
        static Awaitable<void> get_return_object_on_allocation_failure();

        FinalAwaiter<Promise> final_suspend() noexcept { return FinalAwaiter<Promise>{}; }

        Promise() = default;
        Promise(Promise const&) noexcept = delete;
        Promise& operator=(Promise const&) noexcept = delete;
    };

    template <typename _RetT>
    struct Awaitable
    {
        typedef Promise<_RetT> promise_type;
        coroutine_handle<promise_type> _myHandle = nullptr;          // nullptr indicates allocation failure		

        explicit Awaitable(__in coroutine_handle<promise_type> Handle) noexcept
            :  _myHandle(Handle)
        {
        };

        Awaitable() = default;

        Awaitable(__in const Awaitable& Other) = delete;
        Awaitable& operator=(__in const Awaitable& Other) = delete;

        Awaitable(__in Awaitable&& Other) noexcept
        {
            _myHandle = Other._myHandle;
            Other._myHandle = nullptr;
        }

        Awaitable& operator=(__in Awaitable&& Other) noexcept
        {
            if (&Other != this)
            {
                DestroyHandle();
                _myHandle = Other._myHandle;
                Other._myHandle = nullptr;
            }
            return *this;
        }

        ~Awaitable() noexcept
        {
            DestroyHandle();
        }

        void DestroyHandle()
        {
            if (_myHandle)
            {
                KInvariant(_myHandle.promise()._state.IsResumed());   // Can't leak uncompleted Awaitables - use ktl::Task or add co_await
                                                                      // This occurs when an Awaitable<> is active and has not been completed
                                                                      // via a co_wait before dtor'd. Sometimes the application really whats
                                                                      // to start a fire-and-forget async co_routine. In such cases ktl::Task
                                                                      // is the right form of awaitable to use. In all other cases, it is an
                                                                      // Application BUG to not co_wait an active Awaitable<>.

                _myHandle.destroy();
                _myHandle = nullptr;
            }
        }

        // Detect if actual co_routine is assigned to this awaitable; false could indicate an allocation failure
        explicit operator bool() const noexcept
        {
            return (_myHandle) ? true : false;
        }

        // Waitable behaviors
        bool await_ready() noexcept
        {
            if (!_myHandle)
            {
                return true;        // treat non-co_routine_bound situation as completed (e.g. co_routine Allocation failure)
            }
            return false;           // Always force await_suspend() and FinalAwaiter::await_suspend()
                                    // to settle their race
        }

        bool IsReady()
        {
            if (!_myHandle)
            {
                return true;
            }

            return _myHandle.promise()._state.IsReady();
        }

        bool IsComplete()
        {
            if (!_myHandle)
            {
                return true;                // treat non-co_routine_bound situation as completed (e.g. co_routine Allocation failure)
            }
            return _myHandle.done();
        }

        _RetT await_resume()
        {
            if (!_myHandle)
            {
                throw FrameNotBoundException{};         // Most likely a co_routine frame allocation failure
            }

            if (!_myHandle.promise()._state.IsResumed())
            {
                KInvariant(_myHandle.done());               // Must be done if resumption is being attempted
                _myHandle.promise()._state.SetResumed();    // done this way to help catch illegal parallel callers
            }

            if ((bool)(_myHandle.promise()._exception))
            {
#if !defined(PLATFORM_UNIX)
                _myHandle.promise()._exception._RethrowException();
#else
                rethrow_exception(_myHandle.promise()._exception);
#endif
            }

            return _myHandle.promise().value();
        }

        bool await_suspend(__in coroutine_handle<> Handle) noexcept
        {
            if (!_myHandle)
            {
                return false;       // treat non-co_routine_bound situation as completed (e.g. co_routine Allocation failure)
            }

            if (_myHandle.promise()._state.IsResumed())
            {
                // allow for extra calls suspension sequences after completion - given await_ready() always rets false
                return false;
            }

            // Assert if await_suspended has been called and we have reached this point. Protect promise state from a buggy
            // app with racing co_await.
            _myHandle.promise()._state.SetAwaitSuspendActive();

            _myHandle.promise()._awaiterHandle = Handle;    // to be undone if not suspended
#if defined(K_UseResumableDbgExt)
                                                            // Record our suspension address and track this awaiting promise - to be undone if not suspended
            _myHandle.promise()._ReturnAddress = ::_ReturnAddress();
            K_LOCK_BLOCK(PromiseCommon::gs_GlobalLock.Lock())
            {
                PromiseCommon::gs_List.InsertTop(_myHandle.promise()._ListEntry);
            }
#endif
            MemoryBarrier();

            KAssert(!_myHandle.promise()._state.IsCalloutOnFinalHeld());     // Application fault: Callout can't be scheduled and co_await issued
            if (_myHandle.promise()._state.TryToSuspend())
            {
                // Ok - suspending - this suspension path raced ahead of the completion path; 
                // defer continuance to FinalAwaiter::await_suspend()
                return true;
            }

            // Not suspending - completion path has raced ahead but of course did not attempt to resume
            KInvariant(_myHandle.promise()._state.IsReady());
            _myHandle.promise()._awaiterHandle = nullptr;

#if defined(K_UseResumableDbgExt)
            // UNDO: Untrack this awaiting promise
            _myHandle.promise()._ReturnAddress = nullptr;
            K_LOCK_BLOCK(PromiseCommon::gs_GlobalLock.Lock())
            {
                _myHandle.promise()._ListEntry.Remove();
            }
#endif

            return false;
        }

        // Try and schedule a completion callback; returns true is callback set; false Awaitable was completed already
        //
        // CalloutOnFinalHeld is auto cleared by a completing thread just before callback is made
        bool TrySetFinalHeldCallback(PromiseCommon::AwaitableCompletedCallback Callback, void* Ctx)
        {
            KInvariant(_myHandle && !_myHandle.promise()._state.IsCalloutOnFinalHeld());

            _myHandle.promise()._CompletedCallback = Callback;
            _myHandle.promise()._CompletedCallbackCtx = Ctx;
            return _myHandle.promise()._state.TryToScheduleCalloutOnFinalHeld();
        }

        // returns true iif FinalHeldCallback was set and was cleared by this call
        bool ClearFinalHeldCallback()
        {
            KInvariant(_myHandle);
            return _myHandle.promise()._state.ResetCalloutOnFinalHeld();
        }

        bool IsExceptionCompletion()
        {
            if (!_myHandle)
            {
                return true;                // treat non-co_routine_bound situation as completed (e.g. co_routine Allocation failure)
            }
            KInvariant(_myHandle.done());
            return (bool)(_myHandle.promise()._exception);
        }

        ExceptionHandle GetExceptionHandle()
        {
            if (!_myHandle)
            {
                return std::make_exception_ptr<FrameNotBoundException>(FrameNotBoundException{});
            }
            KInvariant((bool)(_myHandle.promise()._exception));
            return _myHandle.promise()._exception;
        }

        AwaitableState GetState() { return _myHandle.promise()._state; }
    };

    static_assert((sizeof(Awaitable<int>) == 8), "ktl::Awaitable<T> too large");
    static_assert((sizeof(Awaitable<void>) == 8), "ktl::Awaitable<void> too large");

    template <typename T>
    inline Awaitable<T> Promise<T>::get_return_object()
    {
        return Awaitable<T>(coroutine_handle<Promise<T>>::from_promise(*this));
        // NOTE: The ctor'd Awaitable is done in-place inside of the calling coroutine's supplied memory
        //       -- typically as a local
    }

    // Called when allocation fails in operator new above
    template <typename T>
    inline Awaitable<T> Promise<T>::get_return_object_on_allocation_failure()
    {
        return Awaitable<T>();
    }

    inline Awaitable<void> Promise<void>::get_return_object()
    {
        return Awaitable<void>(coroutine_handle<Promise<void>>::from_promise(*this));
        // NOTE: The ctor'd Awaitable is done in-place inside of the calling coroutine's supplied memory
        //       -- typically as a local
    }

    // Called when allocation fails in operator new above
    inline Awaitable<void> Promise<void>::get_return_object_on_allocation_failure()
    {
        return Awaitable<void>();
    }


    //* Coroutine optimized async lock
    //
    // Description: In a pure coroutine environment when running mostly on ktl threads or you have a known worst case recursion limit,
    // this async lock imp will be better perf and result in fewer heap objects being allocated.
    //
    struct KCoLock
    {
    private:
        // Stall continuance until Callback() or Continuation::Execute()
        __inline static ktl::Awaitable<void> Staller()
        {
            co_await suspend_always{};
            co_return;
        }

    public:
        KCoLock(__in KtlSystem& Sys) : _system(Sys) {}

        ~KCoLock()
        {
            KInvariant(!_isOwned && _awaiters.IsEmpty());
        }

        //* Acquire Lock
        ktl::Awaitable<void> AcquireAsync()
        {
            Continuation            continuation;
            ktl::Awaitable<void>    staller;

            K_LOCK_BLOCK(_myLock)
            {
                if (!_isOwned)
                {
                    KAssert(_awaiters.IsEmpty());
                    _isOwned = true;
                    co_return;
                }

                // Suspend this acquire until it is its turn 
                _awaiters.InsertBottom(continuation._awaitersEntry);

                // capture coroutine_handle<> for Staller()
                staller = Staller();
                continuation._crHandle = staller._myHandle;
            }

            co_await staller;           // Stall
            co_return;
        }

        // Release a lock that was acquired via AcquireAsync() - returns false on OOM condition
        //
        // Paramemters: AllowRecursion (optional): Defaults to false. When true, resumption of the next
        //                                         awaiting coroutine will occur sync to the caller's thread.
        //                                         As such recursion can occur and thus stack overflow could occur
        //                                         unless the caller can guarantee max depth. This mode, if such 
        //                                         a guarantee can be made is very performant. 
        //
        //                                         When runing on a KTL thread, there is a very little perf value
        //                                         to be had by passing true.
        //
        bool Release(__in_opt bool AllowRecursion = false)
        {
            LinkListEntry*      nextOwner = nullptr;
            bool                doScheduleOnSysTP = ((!AllowRecursion) && (!_system.IsCurrentThreadOwned()));
            PTP_WORK            work = nullptr;

            K_LOCK_BLOCK(_myLock)
            {
                KInvariant(_isOwned);
                _isOwned = false;
                if (_awaiters.IsEmpty())
                {
                    // No awaiters
                    return true;
                }

                // Let next _awaiter run
                nextOwner = &_awaiters.GetTop().Remove();
                _isOwned = true;

                if (doScheduleOnSysTP)
                {
                    // Must be done under lock in case we need to backout and we only now know the nextOwner
                    work = CreateThreadpoolWork(Callback, Continuation::FromLinkAddress(*nextOwner), nullptr);
                    if (work == nullptr)
                    {
                        // #1 must backout scheduling next awaiter
                        &_awaiters.InsertTop(*nextOwner);
                    }
                }
            }

            if (AllowRecursion)
            {
                Continuation::FromLinkAddress(*nextOwner)->_crHandle.resume();
                return true;
            }

            // continue from top of stack
            if (!doScheduleOnSysTP)
            {
                // Defer to top of this ktl thread stack
                _system.DefaultSystemThreadPool().QueueWorkItem(*Continuation::FromLinkAddress(*nextOwner), TRUE);
            }
            else
            {
                // Continuance must done on sys TP worker thread - "work" was allocated under lock above
                if (work == nullptr)
                {
                    // Was undone because of #1 above 
                    return false;
                }

                SubmitThreadpoolWork(work);
            }

            return true;
        }

    private:
        struct Continuation : public KThreadPool::WorkItem
        {
            LinkListEntry       _awaitersEntry;             // see KCoLock::_awaiters
            coroutine_handle<>  _crHandle = nullptr;

            static Continuation* FromLinkAddress(LinkListEntry& LinkAddr)
            {
                return (Continuation*)(((char*)(&LinkAddr)) - offsetof(Continuation, _awaitersEntry));
            }

            void Execute() override
            {
                _crHandle.resume();
            }
        };

    private:
        KCoLock() = delete;

        // continue from a sys TP worker thread
        static void Callback(PTP_CALLBACK_INSTANCE, PVOID Param, PTP_WORK Work)
        {
            CloseThreadpoolWork(Work);
            ((Continuation*)Param)->_crHandle.resume();
        }

    private:
        KSpinLock       _myLock;
        LinkListHeader  _awaiters;
        bool            _isOwned = false;
        KtlSystem&      _system;
    };


    //* General helper to await for either of two Awaitables to be in a completed (ready) state
    //
    //	Usage: co_await EitherReady(Awaitable<T|void>& Aw1, Awaitable<T|void>& Aw2);
    //         if (Aw1.IsReady()) co_wait Aw1; ...;
    //         if (Aw2.IsReady()) co_wait Aw2; ...;
    //
    template <typename _AwrT1, typename _AwrT2>
    ktl::Awaitable<void> EitherReady(KtlSystem& Sys, typename _AwrT1& Awaiter1, typename _AwrT2& Awaiter2)
    {
        bool ready[] = { Awaiter1.IsReady(), Awaiter2.IsReady() };
        if (std::any_of(std::begin(ready), std::end(ready), [](bool b) {return b; }))
        {
#if defined(KtlEitherReady_TestFastPath)
            if (::_g_KtlEitherReady_TestFastPath)
            {
                // Quick path
                co_return;
            }
#else
            // Quick path
            co_return;
#endif
        }

        // Dummy coroutine that works as a stall point
        static auto SuspendUntil = []() -> ktl::Awaitable<void>
        {
            co_await suspend_always();
            co_return;
        };

        ktl::Awaitable<void>    staller = SuspendUntil();           // Start a suspended coroutine
        CHAR                    firstAwaiter = false;               // set true by first completing awaitable

                                                                    // Callback Binding w/callback handler
        struct CallbackBinding
        {
            coroutine_handle<>      _stallerCRHandle = nullptr;
            KCoLock                 _awaiterCallbackDone;
            CHAR&                   _firstAwaiter;

            CallbackBinding() = delete;

            CallbackBinding(KtlSystem& Sys, coroutine_handle<> Stallerhandle, CHAR& FirstAwaiter)
                : _stallerCRHandle(Stallerhandle),
                _firstAwaiter(FirstAwaiter),
                _awaiterCallbackDone(Sys)
            {
            }

            static void Callback(void* Ctx)         // Awaitable<> ready callback - see TrySetFinalHeldCallback()
            {
                CallbackBinding&       _this = *((CallbackBinding*)Ctx);

                bool wasSet = (bool)InterlockedExchange8(&_this._firstAwaiter, CHAR(true));
                if (!wasSet)
                {
                    // First thru - cause staller to unstall pending co_await
                    _this._stallerCRHandle.resume();
                }
                KInvariant(_this._awaiterCallbackDone.Release(true));       // signal this callback is done with "*_this" and local frame state
            }
        };

        CallbackBinding     bindings[] =
        {
            {Sys, staller._myHandle, firstAwaiter},
            {Sys, staller._myHandle, firstAwaiter}
        };

        for (auto& binding : bindings)
        {
            co_await binding._awaiterCallbackDone.AcquireAsync();      // preacquire; done so lock (_awaiterCallbackDone) can be used as an event
        }

        // Attempt to schedule completion-held callbacks on passed Awaitable<>s
        bool awaiterHeldCallbackScheduled[] =
        {
            Awaiter1.TrySetFinalHeldCallback(bindings[0].Callback, &bindings[0]),
            Awaiter2.TrySetFinalHeldCallback(bindings[1].Callback, &bindings[1])
        };

        bool awaitStaller = false;
        bool resume = false;
        // At this point each awaiter can: 1) be complete already (!awaiterXHeldCallbackScheduled), or 2) racing to call its callback once completed
        if (std::all_of(std::begin(awaiterHeldCallbackScheduled), std::end(awaiterHeldCallbackScheduled), [](bool b) {return b; }))
        {
            // Both were scheduled; wait for first one to callback
            co_await staller;
            KAssert(firstAwaiter);
        }
        else
        {
            awaitStaller = true;
            resume = true;
        }

        bool awaiterWasHeld[] =
        {
            awaiterHeldCallbackScheduled[0] ? Awaiter1.ClearFinalHeldCallback() : false,
            awaiterHeldCallbackScheduled[1] ? Awaiter2.ClearFinalHeldCallback() : false
        };

        // Let any in-progress callback finish (with bound locals)
        for (int i = 0; i < 2; ++i)
        {
            if (awaiterHeldCallbackScheduled[i])
            {
#if defined(KtlEitherReady_TestHoldAwDelay)
                if (awaitStaller && (::_g_KtlEitherReady_TestHoldAwDelay > 0))
                {
                    KNt::Sleep(::_g_KtlEitherReady_TestHoldAwDelay);
                }
#endif
                if (!awaiterWasHeld[i])
                {
                    // awaiter did/is going to call back - wait until callback done -- test path #2
                    co_await bindings[i]._awaiterCallbackDone.AcquireAsync();
                    KAssert(firstAwaiter);                  // required: callback resumed staller()
                    resume = false;
                }
            }
        }

        if (resume)
        {
            KAssert(!firstAwaiter);                 // callback did not resume staller()
                                                    // both awaiters completed before we could set the held-callbacks
            staller._myHandle.resume();             // make sure we can fall thru the co_await below
        }

        // undo preacquire
        for (auto& binding : bindings)
        {
            KInvariant(binding._awaiterCallbackDone.Release(true));    // undo preacquires
        }

        if (awaitStaller)
        {
            co_await staller;
        }

        KAssert(!Awaiter1.GetState().IsCalloutOnFinalHeld() && !Awaiter2.GetState().IsCalloutOnFinalHeld());
        co_return;
    }
    
    //* General helper to convert an Awaitable to a Task at runtime
    //      Provides a means to hand off completion to a background Task 
    //
    //	Usage: ToTask(Awaitable<T|void>(...));
    //
    template <typename _AwrT>
    Task ToTask(_AwrT&& A)
    {
        auto awaitable = Ktl::Move(A);      // Take the contents of A
        if (awaitable)
        {
            try
            {
                co_await awaitable;
            }
            catch (...)
            {
                // Todo: Consider what if anything to do.
            }
        }
    }

    //* General thread sync awaitor function - Thanks Gor (gorn)!
    //
    //	Usage: T|void x = SyncWait(Awaitable<T|void>(...))
    //
    template <typename _AwrT>
    auto SyncAwait(_AwrT&& A)
    {
        KAssert(!KtlSystem::IsCurrentThreadOwned());
        if (!A.await_ready())
        {
            KEvent taskJoinedEvent;

            auto AwaitAsync = [&]() -> Task
            {
                try
                {
                    co_await A;
                }
                catch (...)
                {
                    // Will be rethrown at #1 below
                }
            
                taskJoinedEvent.SetEvent();
            };

            Task t = AwaitAsync();
            KInvariant(t.IsTaskStarted());
            taskJoinedEvent.WaitUntilSet();
        }
        return A.await_resume();      // #1
    }

    //** Static co_routine helpers
    class CorHelper
    {
    public:
        //* Helper to gain access to a current coroutine's own Promise
        //
        //  usage: auto&  myPromise = co_await CorHelper::GetCurrentPromise();
        //
        template <typename TPromise>
        struct GetCurrentPromiseAwaiter
        {
            TPromise* me;
            bool await_ready() { return false; }
            void await_suspend(std::experimental::coroutine_handle<TPromise> h) {
                me = &h.promise();
                h.resume();
            }
            TPromise* await_resume() { return me; }
        };

        template <typename TPromise>
        static GetCurrentPromiseAwaiter<TPromise> GetCurrentPromise()
        {
            return GetCurrentPromiseAwaiter<TPromise>();
        }

        //* Helper to guarantee after continuance on a non-ktl thread.
        //
        //	Usage: bool worked = co_await ktl::ContinueOnNonKtlThread(myKtlSystem);
        //
        // Returns: true    - on non-ktl thread
        //          false   - could not schedule non-ktl thread for continuance
        //
        static Awaitable<bool> ContinueOnNonKtlThread(KtlSystem& Sys)
        {
            struct DeferredCallback : public KThreadPool::WorkItem
            {
            public:
                coroutine_handle<>  _coToContinue = nullptr;
                bool                _WasScheduled = false;
                KSpinLock           _callbackThreadHold;
                PTP_WORK_CALLBACK   _startFunc;

                // Guarenteeded to be called after the current ktl thread has returned from QueueUserWorkItem
                // because of co_await suspension
                void Execute() override
                {
                    // We must schedule a system TP thread to resume activity and get that indication 
                    // before it is allowed to run
                    _callbackThreadHold.Acquire();

                    PTP_WORK work = CreateThreadpoolWork(_startFunc, this, nullptr);
                    _WasScheduled = (work != nullptr);

                    if (_WasScheduled)
                    {
                        SubmitThreadpoolWork(work);
                        work = nullptr;
                    }
                    _callbackThreadHold.Release();      // let _startFunc() run
                    if (!_WasScheduled)
                    {
                        // Could not schedule the sys TP work item - force resumption
                        _coToContinue.resume();
                    }
                }

            } deferedCallback;

            static auto callback = [](_TP_CALLBACK_INSTANCE*, void* Param, _TP_WORK* Work) -> void
            {
                DeferredCallback&       callbackState = *((DeferredCallback*)Param);

                // Sync to Execute() realizing result of QueueUserWorkItem();
                callbackState._callbackThreadHold.Acquire();
                callbackState._callbackThreadHold.Release();

                CloseThreadpoolWork(Work);
                callbackState._coToContinue.resume();               // Continue at #1
            };

            static auto SuspendUntil = []() -> ktl::Awaitable<void>
            {
                co_await suspend_always();
                co_return;
            };

            //* If already on non-ktl thread - don't bother
            if (!Sys.IsCurrentThreadOwned())
            {
                // Already on non-ktl thread
                co_return true;
            }

            // Start stalled co_routine; only completes when callback() does explicit resumption
            // after co_await below has returned this caller's thread
            ktl::Awaitable<void> x = SuspendUntil();

            // callback() and deferedCallback::Execute() Params
            deferedCallback._coToContinue = x._myHandle;
            deferedCallback._startFunc = callback;

            // deferedCallback::Execute() is guaranteeed to NOT to be called until after the currect KTL thread 
            // returns from within the co_await below - in suspension
            Sys.DefaultSystemThreadPool().QueueWorkItem(deferedCallback, TRUE);
            co_await x;
            //* Continued from #1: On TP worker thread

            co_return deferedCallback._WasScheduled;
        }

        //* Helper to continue the current coroutine execution after being
        //  dispatched via a thread supplied by the passed KThreadPool
        //
        //  usage example: 
        //      co_await CorHelper::ThreadPoolThread(GetThisKtlSystem().DefaultSystemThreadPool());
        //
        #pragma optimize("", off)
        struct ThreadPoolThreadAwaitable;
        static __declspec(noinline) ThreadPoolThreadAwaitable ThreadPoolThread(KThreadPool& ThreadPoolToUse)
        {
            return ThreadPoolThreadAwaitable(ThreadPoolToUse);
        }

        struct ThreadPoolThreadAwaitable : private KThreadPool::WorkItem
        {
        private:
            coroutine_handle<>  _myHandle = nullptr;
            KThreadPool*        _threadPoolToUse = nullptr;

            __declspec(noinline) void Execute() override
            {
                coroutine_handle<> handle = _myHandle;
                _myHandle = nullptr;
                MemoryBarrier();

                handle.resume();
            }

            // No default or copy ctor, or copy/move ops
            ThreadPoolThreadAwaitable() = delete;
            ThreadPoolThreadAwaitable(ThreadPoolThreadAwaitable&) = delete;
            ThreadPoolThreadAwaitable& operator=(ThreadPoolThreadAwaitable const&) = delete;
            ThreadPoolThreadAwaitable& operator=(ThreadPoolThreadAwaitable const&&) = delete;

        public:
            __declspec(noinline) ThreadPoolThreadAwaitable(KThreadPool& ThreadPoolToUse)
            {
                _threadPoolToUse = &ThreadPoolToUse;
            }

            __declspec(noinline) ~ThreadPoolThreadAwaitable()
            {
                KInvariant(!_myHandle);			// must be co_awaited on
            }

            __declspec(noinline) ThreadPoolThreadAwaitable(ThreadPoolThreadAwaitable&& Source)
            {
                _threadPoolToUse = Source._threadPoolToUse;
                _myHandle = Source._myHandle;
                Source._threadPoolToUse = nullptr;
                Source._myHandle = nullptr;
            }

            // co_await contract
            bool await_ready() { return false; }

            __declspec(noinline) void await_suspend(coroutine_handle<> h)
            {
                _myHandle = h;
                MemoryBarrier();

                _threadPoolToUse->QueueWorkItem(*this);
            }

            void await_resume() {}
        };
        #pragma optimize("", on)
    };

    namespace kasync
    {
        using ::_delete;

        //* Common KAsyncContextBase derivation co_routine support class. See KTimer.h for usage example.
        class InplaceStartAwaiter : public KObject<InplaceStartAwaiter>
        {
        public:
            InplaceStartAwaiter(
                __in KAsyncContextBase& Async,
                __in_opt KAsyncContextBase* const Parent,
                __in_opt KAsyncGlobalContext* const GlobalContext)
            {
                Reuse(Async, Parent, GlobalContext);
            }

            InplaceStartAwaiter()
            {
                KInvariant(!_awaiterHandle);
                _async = nullptr;
                _parentAsync = nullptr;
                _globalContext = nullptr;
                _status = STATUS_SUCCESS;
                _doStart = TRUE;
            }

            ~InplaceStartAwaiter();

            void SetDoStart(BOOLEAN DoStart)
            {
                _doStart = DoStart;
            }

            void Reuse(
                __in KAsyncContextBase& Async,
                __in_opt KAsyncContextBase* const Parent,
                __in_opt KAsyncGlobalContext* const GlobalContext)
            {
                KInvariant(!_awaiterHandle);
                _async = &Async;
                _parentAsync = Parent;
                _globalContext = GlobalContext;
                _status = STATUS_SUCCESS;
                _doStart = TRUE;
                _state.Reuse();
            }

            bool await_ready() { return false; }

            NTSTATUS await_resume()
            {
                return _status;
            }

            bool await_suspend(__in coroutine_handle<> CrHandle)
            {
                return DoAwaitSuspend(CrHandle);
            }

            KAsyncContextBase::CompletionCallback GetInternalCallback()
            {
                KAsyncContextBase::CompletionCallback callback;
                callback.Bind(this, &InplaceStartAwaiter::OnCompletionCallback);

                return callback;
            }

        protected:
            bool DoAwaitSuspend(__in coroutine_handle<> CrHandle)
            {
                // Assert if await_suspended has been called and we have reached this point. Protect promise state from a buggy
                // app with racing co_await.
                _state.SetAwaitSuspendActive();

                KInvariant(!_awaiterHandle);
                _awaiterHandle = CrHandle;                  // Assume we are going to suspend

                if (_doStart)
                {
                    class PrivateAsyncDef : public KAsyncContextBase
                    {
                    public:
                        using KAsyncContextBase::Start;
                    };

                    // Start the operation
                    KAsyncContextBase::CompletionCallback callback;
                    callback.Bind(this, &InplaceStartAwaiter::OnCompletionCallback);

                    ((PrivateAsyncDef&)(*_async)).Start(
                        _parentAsync,
                        callback,
                        _globalContext);
                }

                // NOTE: "this" must (will) stay stable until return from this function
                if (_state.TryToSuspend())
                {
                    // Ok - suspending - this suspension path raced ahead of the completion path; 
                    // defer continuance to OnCompletionCallback()
                    return true;
                }   

                // The OnCompletionCallback() path raced ahead - we are completing sync
                _awaiterHandle = nullptr;           // Undo the suspension assumption
                return false;
            }

        private:
            void OnCompletionCallback(
                __in_opt KAsyncContextBase* const,           // Parent Async Awaiter; can be nullptr
                __in KAsyncContextBase& CompletingAsync) noexcept
            {
                _status = CompletingAsync.Status();
                KInvariant((!_doStart) || !_awaiterHandle.done());      // !_doStart can indicate that _awaiterHandle has not been set yet
                                                                        // we depend on _state to order access
                _async = nullptr;
                _parentAsync = nullptr;
                _globalContext = nullptr;

                // Defer continuance thru await_suspend() if this completion path raced ahead
                if (_state.TryToFinalize())
                {
                    return;     // continuance will occur via await_suspend()
                }

                // await_suspend() raced ahead and suspended - we'll resume on this path
                KAssert(!_awaiterHandle.done());

                coroutine_handle<>      awaiterHandle = _awaiterHandle;
                _awaiterHandle = nullptr;
                awaiterHandle.resume();             // continue sync @ this->await_resume()
                                                    // !NOTE: 'this' not valid from this point
            }

        private:
            KAsyncContextBase*      _async;
            KAsyncContextBase*      _parentAsync;
            KAsyncGlobalContext*    _globalContext;
            coroutine_handle<>      _awaiterHandle = nullptr;
            AwaitableState          _state;
            NTSTATUS                _status;
            BOOLEAN                 _doStart;
        };

        class StartAwaiter : public InplaceStartAwaiter, public KShared<StartAwaiter>
        {
            K_FORCE_SHARED(StartAwaiter);

        public:
            // Add static factory
            static __declspec(noinline)
            NTSTATUS Create(__in KAllocator& Allocator,
                    __in ULONG Tag,
                    __in KAsyncContextBase& Async,
                    __out StartAwaiter::SPtr& Result,
                    __in_opt KAsyncContextBase* const Parent = nullptr,
                    __in_opt KAsyncGlobalContext* const GlobalContext = nullptr)
            {
                Result = _new(Tag, Allocator) StartAwaiter(Async, Parent, GlobalContext);
                if (!Result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(Result->Status()))
                {
                    return (Ktl::Move(Result))->Status();
                }

                return STATUS_SUCCESS;
            }

            bool await_suspend(__in coroutine_handle<> CrHandle)
            {
                StartAwaiter::SPtr      thisPtr = this;         // keep stable until return
                return DoAwaitSuspend(CrHandle);
            }

        private:
            StartAwaiter(
                __in KAsyncContextBase& Async,
                __in_opt KAsyncContextBase* const Parent,
                __in_opt KAsyncGlobalContext* const GlobalContext)
                : InplaceStartAwaiter(Async, Parent, GlobalContext)
            {
            }
        };
    }
}
#endif

// Standard entry macros for all Task, and Awaitable instance methods on KAsyncs and simple KShared
// objects. 
#define __KAsync$ApiEntry(ClosedRetExp) if (!this->TryAcquireActivities()) ClosedRetExp; KFinally([&] { this->ReleaseActivities(); });
#define __KAsync$VoidEntry() this->AcquireActivities(); KFinally([&] { this->ReleaseActivities(); });
#define __KShared$ApiEntry() this->AddRef(); KFinally([&] { this->Release(); });

// non-coroutine versions.
#define KAsync$ApiEntry() __KAsync$ApiEntry(return K_STATUS_API_CLOSED);
#define KAsync$VoidEntry() __KAsync$VoidEntry();
#define KShared$ApiEntry() __KShared$ApiEntry();

// coroutine versions
#if defined(K_UseResumable)
#define KCoAsync$ApiEntry() __KAsync$ApiEntry(co_return {(NTSTATUS)K_STATUS_API_CLOSED});
#define KCoAsync$TaskEntry() __KAsync$VoidEntry(); 
#define KCoShared$ApiEntry() __KShared$ApiEntry();
#endif
