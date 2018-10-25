/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KTimer.h

    Description:
      Kernel Template Library (KTL): One shot Timer

    History:
      richhas          21-Oct-2010         Initial version.

--*/

#pragma once


//** KTimer Definitions
static const ULONGLONG KTimerTypeName = K_MAKE_TYPE_TAG('  re', 'miTK');

class KTimer : public KAsyncContextBase, public KTagBase<KTimer, KTimerTypeName>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KTimer);

public:
    // Factory's for KTimer instances
    static NTSTATUS
    Create(KTimer::SPtr& Result, KAllocator& Allocator, ULONG Tag);

    // Start a timer operation
    //
    // Parameters:
    //     MSecsToWait - Minimum number of msecs to delay before this instance completes.
    //                   A completion status of STATUS_SUCCESS indicates the timer fired.
    //     Parent      - optional parent async context - see KAsyncContext.h
    //     Callback    - optional completion callback delegate - see KAsyncContext.h
    //
    // Notes:
    //      A current operation may be aborted via a call to Cancel(). In that case a completion
    //      status of STATUS_CANCELLED could occur.
    VOID
    StartTimer(
        ULONG MSecsToWait,
        KAsyncContextBase* const Parent,
        KAsyncContextBase::CompletionCallback Callback);

#if defined(K_UseResumable)
	// Start an awaitable timer operation
	//
	// Parameters:
	//     MSecsToWait - Minimum number of msecs to delay before this instance completes.
	//                   A completion status of STATUS_SUCCESS indicates the timer fired.
	//     Parent      - optional parent async context - see KAsyncContext.h
	//
	// Notes:
	//      A current operation may be aborted via a call to Cancel(). In that case a completion
	//      status of STATUS_CANCELLED could occur.
	//
	//	Usage:
	//			NTSTATUS status = co_await myTimer->StartTimeAsync(4500, nullptr);		// wait for 4.5 secs
	//				-OR-
	//			NTSTATUS status = co_await KTimer::StartTimeAsync(allocator, allocTag, 200, nullptr);		// wait for 200 msecs
	//
	//

    ktl::Awaitable<NTSTATUS> StartTimerAsync(
        __in ULONG MSecsToWait, 
        __in_opt KAsyncContextBase* const Parent, 
        __in_opt KAsyncGlobalContext* const GlobalContext = nullptr) noexcept
    {
        // DEBUG: Trap promise corruption
        auto& myPromise = *co_await ktl::CorHelper::GetCurrentPromise<ktl::Awaitable<NTSTATUS>::promise_type>();
        auto myState = myPromise.GetState();
        KInvariant(myState.IsUnusedBitsValid());
        KInvariant(!myState.IsReady());
        KInvariant(!myState.IsSuspended());
        KInvariant(!myState.IsFinalHeld());
        KInvariant(!myState.IsCalloutOnFinalHeld());

        _MSecsToWait = MSecsToWait;

        ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), Parent, GlobalContext);

        NTSTATUS status = co_await awaiter;

        // DEBUG: Trap promise corruption
        myState = myPromise.GetState();
        KInvariant(myState.IsUnusedBitsValid());
        KInvariant(!myState.IsReady());
        KInvariant(!myState.IsFinalHeld());

        co_return status;
    }

    static ktl::Awaitable<NTSTATUS> StartTimerAsync(
        __in KAllocator& Allocator,
        __in ULONG Tag,
        __in ULONG MSecsToWait,
        __in_opt KAsyncContextBase* const Parent,
        __in_opt KAsyncGlobalContext* const GlobalContext = nullptr) noexcept
    {
        // DEBUG: Trap promise corruption
        auto& myPromise = *co_await ktl::CorHelper::GetCurrentPromise<ktl::Awaitable<NTSTATUS>::promise_type>();
        auto myState = myPromise.GetState();
        KInvariant(myState.IsUnusedBitsValid());
        KInvariant(!myState.IsReady());
        KInvariant(!myState.IsSuspended());
        KInvariant(!myState.IsFinalHeld());
        KInvariant(!myState.IsCalloutOnFinalHeld());
        
        KTimer::SPtr	timer;

        NTSTATUS status = KTimer::Create(timer, Allocator, Tag);
        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        auto timerAsync = timer->StartTimerAsync(MSecsToWait, Parent, GlobalContext);
        status = co_await timerAsync;

        // DEBUG: Trap promise corruption
        myState = myPromise.GetState();
        KInvariant(myState.IsUnusedBitsValid());
        KInvariant(!myState.IsReady());
        KInvariant(!myState.IsFinalHeld());

        co_return status;
    }

    #endif

    // Reset a timer operation
    //
    // Parameters:
    //     MSecsToWait - Minimum number of msecs to delay before this instance completes.
    //
    // Notes:
    //      Reset will only succeed if a current operation is in progress and hasn't fired.
    BOOLEAN
    ResetTimer(ULONG MSecsToWait);

    static const LONGLONG TicksPerMillisecond = 10000;

protected:
    //** Derivation extension interface
    // Default behavior: ScheduleTimer() is called. It is the responsibility of the overridden implementation
    //                   to start the underlying timer - see ScheduleTimer().
    virtual VOID
    OnStart() override;

    // Default behavior: CancelTimerAndComplete(STATUS_CANCELLED) is called. It is the responsibility of
    //                   the overridden implementation to cancel the underlying timer - see CancelTimer()
    //                   and CancelTimerAndComplete().
    virtual VOID
    OnCancel() override;

    // Default behavior: Ensures the underlying timer is cancelled, so the timer can be safely reused.
    virtual VOID
    OnCompleted() override;

    // Default behavior: Prepares the timer for reuse.
    virtual VOID
    OnReuse() override;

    // Overridable derivation method that is called each time the underlying system timer associated
    // with a KTimer is fired. This call is made prior to the KTimer instance's Complete() method
    // being invoked. In other words once control is received at this method, the callee is guarenteed
    // that the completion of the subject KTimer instance will be held until control is returned from
    // this method.
    //
    // Default behavior: Complete(STATUS_SUCCESS) is called. It is the responsibility of the overridden
    //                   implementation to call Complete().
    virtual VOID
    UnsafeOnTimerFiring();

protected:
    //** Derivation support interface

    // Starts the underlying system timer with _MSecsToWait being the duration.
    VOID
    ScheduleTimer();

    // Cancels the underlying system timer and will attempt to complete the current
    // operation with the passed Status value.
    VOID
    CancelTimerAndComplete(NTSTATUS Status);

    // Cancels the underlying system timer an returns an indication if the caller
    // should take responsibiliy for calling Complete().
    BOOLEAN
    CancelTimer();

#if KTL_USER_MODE
#if defined(PLATFORM_UNIX)
    static VOID CALLBACK
    TimerCallback(union sigval Context);
#else
    static VOID CALLBACK
    TimerCallback(
        __inout PTP_CALLBACK_INSTANCE   Instance,
        __inout_opt PVOID               Context,
        __inout PTP_TIMER               Timer);
#endif
#else
    static KDEFERRED_ROUTINE DpcCallback;
#endif

protected:
    ULONG               _MSecsToWait;

private:
    volatile SHORT      _TimerFired;

#if KTL_USER_MODE
#if defined(PLATFORM_UNIX)
    timer_t             _Timer;
#else
    PTP_TIMER           _TimerPtr;
#endif
#else
    KTIMER              _KTimer;
    KDPC                _KDpc;
#endif
};

//
// The KAdaptiveTimer class implements a adaptive timer which adjusts the time out
// value based on how long previous times took.  Basically the timer maintains a
// running average of the previous time intervals.
//
// The timer is implemented as two separate objects, the first is the Root or Base
// part of which holds the default parameters and the running or average interval
// time. The second is an actual per instance timer. This object maintains the time
// the operation took as well as the last timeout value that was used.  When a
// timeout occurs the timer will used a larger timeout for the next interation.
//
// The adaptive timer has a large number of configurable parameters which control its
// behavior:
//  MaximumTimeout - Specifies the maximum timeout in milliseconds that will be used
//      regardless of the number of retries.
//
//  MimimumTimeout - Specifies the minimum timeout in milliseconds that will be
//      used regardless of the previous interval times.
//
//  InitialTimeInterval - Specifies the initial time interval in milliseconds when
//      first starting.  This is used to calculate the initial timesout values
//
//  WeightOfExistingTime - Specifies the numerator of the ratio over 256 to use
//      when adding a new interval time to the previous time.  PreviousTime =
//      ((PreviousTime WeightOfExistingTime) + (NewTime * (256 -
//      WeightOfExistingTime))/256. Typically 224 (7/8) is used.
//
//  TimeToTimeoutMultipler - Used to calculate the initial timeout value based on
//      previous times.  InitialTimeOut = PreviousTIme * TimeToTimeoutMultipler.
//      Typically 4 is used.
//
//  BackoffMultipler - Used to calculate the how much longer to wait after a time
//      a timeout. Typically 2 is used.
//
//  The default values are filled in from the Parameters structure and canbe modified
//  by command line strings.
//

static const ULONGLONG KAdaptiveTimerRootTypeName = K_MAKE_TYPE_TAG('tRmi', 'TpdA');

class KAdaptiveTimerRoot :
    public KShared<KAdaptiveTimerRoot>,
    public KObject<KAdaptiveTimerRoot>,
    public KTagBase<KAdaptiveTimerRoot, KAdaptiveTimerRootTypeName>
{
    K_FORCE_SHARED(KAdaptiveTimerRoot);

public:

    class Parameters
    {
        ULONG _MaximumTimeout;
        ULONG _MinimumTimeout;
        ULONG _InitialTimeInterval;
        ULONG _WeightOfExistingTime;
        ULONG _TimeToTimeoutMultipler;
        ULONG _BackoffMultipler;

        friend class KAdaptiveTimerRoot;

    public:

        //
        // This routine is used to set reasonable default parameters.  This used
        // rather than a constructor so the parameters class be used in a driver
        // global.
        //

        VOID InitializeParameters();

        VOID
        ModifyParameters(
            __in ULONG Count,
            __in_ecount(Count) LPCWSTR ModifyStrings[]
            );
    };

    static const ULONGLONG AdaptiveTimerInstanceTypeName = K_MAKE_TYPE_TAG('nImi', 'TpdA');

    class TimerInstance : public KTimer, public KTagBase<TimerInstance, AdaptiveTimerInstanceTypeName>
    {
        K_FORCE_SHARED(TimerInstance);

        //
        // Make the orignal timer routines private.
        //

        using KTimer::StartTimer;
        using KTimer::Cancel;

    public:

        //
        // Starts the timer.  The time out value is based on the previous history of
        // the timer.
        //

        VOID StartTimer(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase::CompletionCallback Callback
            );

        //
        // Indicates the end of a timed interval.  Must be called before the timer is
        // started again or freed.
        //

        VOID StopTimer();

        BOOLEAN IsTimeIntervalStarted() { return(_StartTime != 0); }

        VOID SetTimerRoot(
            __in KAdaptiveTimerRoot::SPtr NewTimerRoot
            );

    private:
        friend class KAdaptiveTimerRoot;

        TimerInstance(
            __in KAdaptiveTimerRoot* TimerRoot
            );

        volatile LONGLONG _StartTime;
        ULONG _PreviousTimeout;

        KSharedPtr<KAdaptiveTimerRoot> _TimerRoot;

    };

    //
    // Factory for KAdaptiveTimerRoot instances
    //

    static NTSTATUS
    Create(
        __out KSharedPtr<KAdaptiveTimerRoot>& TimerRoot,
        __in Parameters const& InitialParameters,
        __in KAllocator& Allocator,
        __in ULONG Tag
        );

    //
    // Factory for TimerInstance associated with the root.
    //

    NTSTATUS
    AllocateTimerInstance(
        __out KSharedPtr<TimerInstance>& Timer
        );

    ULONG GetCurrentTimeInterval() { return(_CurrentTimeInterval); };

    static const ULONG Denominator = 256;

private:

    KAdaptiveTimerRoot(
        Parameters const& InitialParamers,
        KAllocator& Allocator,
        ULONG Tag
        );

    VOID
    ApplyNewTime(
        ULONG TimeInterval
        );

    ULONG
    GetTimeout(
       ULONG PreviousTimeout
        );

    volatile ULONG _CurrentTimeInterval;

    const ULONG _MaximumTimeout;
    const ULONG _MinimumTimeout;
    const ULONG _WeightOfExistingTime;
    const ULONG _TimeToTimeoutMultipler;
    const ULONG _BackoffMultipler;

    const ULONG _Tag;
};
