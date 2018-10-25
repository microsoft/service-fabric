/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KTimer.cpp

    Description:
      Kernel Template Library (KTL): Timer Implementation

    History:
      richhas          21-Oct-2010         Initial version.

--*/

#include "ktl.h"
#include "string.h"

extern "C" _Check_return_ _CRTIMP int __cdecl _wtoi(_In_z_ const wchar_t *_Str);

#if defined(PLATFORM_UNIX)
#define _wtoi(str) wcstod(str, nullptr);
#endif

//** KTimer Implementation
//
// Theory of operation:
//      During construction any underlying system timer service related data structures are allocated such that past
//      construction no dynamic memory allocation is needed. During destruction we are guaranteed that, assuming no
//      reference counting bugs, all underlying system activity related to a current KTimer instance will have
//      completed. This happens because KAsyncContextBase holds a reference on itself (and its optional Parent) while
//      it is in any state active state (i.e. not Operating or Initialized) and thus will only destruct when Completed.
//      This means this instance is not referenced by any system timer facilities as the time of destruction.
//
//      There is an inherent race between system timer callbacks and OnCancel(). This is the reason for the use of
//      _StatusToCompleteWith and in the case of the Kernel target of OnCancel() its conditional call to Complete().
//
NTSTATUS
KTimer::Create(KTimer::SPtr& Result, KAllocator& Allocator, ULONG Tag)
{
    KTimer* result = _new(Tag, Allocator) KTimer();
    Result = result;
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = result->Status();
    if (!NT_SUCCESS(status))
    {
        result = nullptr;
        Result.Reset();     // Should do a release and cause destruction of failed instance
    }
    return status;
}

KTimer::KTimer()
    : _TimerFired(FALSE)
{
#if KTL_USER_MODE
    #if defined(PLATFORM_UNIX)
        sigevent sigEvent = {};
        sigEvent.sigev_notify = SIGEV_THREAD;
        sigEvent.sigev_value.sival_ptr = this;
        sigEvent.sigev_notify_function = &KTimer::TimerCallback;

        int ret = timer_create(CLOCK_MONOTONIC, &sigEvent, &_Timer);
        if (ret != 0)
        {
            _Timer = 0;
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    #else
        _TimerPtr = CreateThreadpoolTimer(&TimerCallback, (PVOID)this, nullptr);
        if (_TimerPtr == nullptr)
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    #endif
#else
        KeInitializeTimer(&_KTimer);
        KeInitializeDpc(&_KDpc, &DpcCallback, (PVOID)this);
#endif
}

KTimer::~KTimer()
{
#if KTL_USER_MODE
    #if defined(PLATFORM_UNIX)
	if (_Timer != 0)
	{
        timer_delete(_Timer);
	}
    #else
        if (_TimerPtr != nullptr)
        {
            CloseThreadpoolTimer(_TimerPtr);
            _TimerPtr = nullptr;
        }
    #endif
#endif
}

VOID
KTimer::StartTimer(
    ULONG MSecsToWait,
    KAsyncContextBase* const Parent,
    KAsyncContextBase::CompletionCallback Callback)
{
    _MSecsToWait = MSecsToWait;
    Start(Parent, Callback);
}

BOOLEAN
KTimer::ResetTimer(ULONG MSecsToWait)
{
    if (TryAcquireActivities())
    {
        KFinally([&]() { ReleaseActivities(); });

        if (CancelTimer())
        {
            _MSecsToWait = MSecsToWait;
            ScheduleTimer();
            return TRUE;
        }
    }

    return FALSE;
}

VOID
KTimer::OnStart()
{
    ScheduleTimer();
}

VOID
KTimer::OnCancel()
{
    // Cancel the timer
    CancelTimerAndComplete(STATUS_CANCELLED);
}

VOID
KTimer::OnReuse()
{
    _TimerFired = FALSE;
}

VOID
KTimer::OnCompleted()
{
    CancelTimer();
}

VOID
KTimer::CancelTimerAndComplete(NTSTATUS Status)
{
    if (CancelTimer())
    {
        Complete(Status);
    }
}

BOOLEAN
KTimer::CancelTimer()
{
#if KTL_USER_MODE
    #if defined(PLATFORM_UNIX)
        itimerspec disarmTimerSpec = {};
        timer_settime(_Timer, 0, &disarmTimerSpec, NULL);
        return !_TimerFired;
    #else
        SetThreadpoolTimer(_TimerPtr, nullptr, 0, 0);
        WaitForThreadpoolTimerCallbacks(_TimerPtr, TRUE);
        return !_TimerFired;
    #endif
#else
        // DPC callback will not be called if TRUE is returned
        return KeCancelTimer(&_KTimer);
#endif
}

VOID
KTimer::ScheduleTimer()
{
    //
    // The the timer is scheduled for 0ms then go ahead and fire it
    //
    if (_MSecsToWait == 0)
    {
        UnsafeOnTimerFiring();
        return;
    }
    
#if defined(PLATFORM_UNIX)
    itimerspec timerSpec = {
        .it_interval = {},
        .it_value = {
            .tv_sec = _MSecsToWait / 1000,
            .tv_nsec = (_MSecsToWait % 1000) * 1000000
        }
    };
    
    timer_settime(_Timer, 0, &timerSpec, NULL);
#else
    LARGE_INTEGER ticksToWait;
    ticksToWait.QuadPart = -1 * (((ULONGLONG)_MSecsToWait) * TicksPerMillisecond);     // To100NanoTicksFromMSecs()

    #if KTL_USER_MODE
        FILETIME t;
        t.dwHighDateTime = ticksToWait.u.HighPart;
        t.dwLowDateTime = ticksToWait.u.LowPart;
        SetThreadpoolTimer(_TimerPtr, &t, 0, 0);
    #else
        BOOLEAN timerWasQueued = KeSetTimer(&_KTimer, ticksToWait, &_KDpc);
        KAssert(!timerWasQueued);
        #if !DBG
        UNREFERENCED_PARAMETER(timerWasQueued);
        #endif
    #endif
#endif
}

// Called by the system timer facility when timer expires:
// NOTE: can be called on any thread - can race with OnCancel() calls
#if KTL_USER_MODE
#if defined(PLATFORM_UNIX)
VOID CALLBACK
KTimer::TimerCallback(union sigval Context)
{
    KTimer* thisPtr = (KTimer*)Context.sival_ptr;
    thisPtr->UnsafeOnTimerFiring();
}
#else
VOID CALLBACK
KTimer::TimerCallback(
    __inout PTP_CALLBACK_INSTANCE   Instance,
    __inout_opt PVOID               Context,
    __inout PTP_TIMER               Timer)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);

    KTimer* thisPtr = (KTimer*)Context;
    thisPtr->UnsafeOnTimerFiring();
}
#endif

#else
VOID
KTimer::DpcCallback(
    __in      struct _KDPC* Dpc,
    __in_opt  PVOID DeferredContext,
    __in_opt  PVOID SystemArgument1,
    __in_opt  PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
    KTimer* thisPtr = (KTimer*)DeferredContext;
    thisPtr->UnsafeOnTimerFiring();
}
#endif

VOID
KTimer::UnsafeOnTimerFiring()
{
    _TimerFired = TRUE;
    Complete(STATUS_SUCCESS);
}

NTSTATUS
KAdaptiveTimerRoot::Create(
    __out KSharedPtr<KAdaptiveTimerRoot>& TimerRoot,
    __in KAdaptiveTimerRoot::Parameters const& InitialParameters,
    __in KAllocator& Allocator,
    __in ULONG Tag
    )
/*++
 *
 * Routine Description:
 *      This function allocates and initializes the KAdaptiveTimerRoot object
 *
 * Arguments:
 *      TimerRoot - Returns the new object if the allocation was successful.
 *
 *      InitialParameters - Supplies the Parameters that should be used for this
 *          timer.
 *
 *       Allocator - Supplies the allocator to use for memory allocations.
 *
 *       Tag - Supplies the allocation tag to use for memory allocations.
 *
 * Return Value:
 *      Returns the status of the operation.
 *
 * Note:
 *
-*/
{

    TimerRoot = _new(Tag, Allocator) KAdaptiveTimerRoot( InitialParameters, Allocator, Tag );
    if (TimerRoot == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = TimerRoot->Status();
    if (!NT_SUCCESS(status))
    {
        TimerRoot.Reset();     // Should do a release and cause destruction of failed instance
    }
    return status;
}

KAdaptiveTimerRoot::KAdaptiveTimerRoot(
    KAdaptiveTimerRoot::Parameters const& InitialParameters,
    KAllocator& Allocator,
    ULONG Tag
    ) :
    _CurrentTimeInterval(InitialParameters._InitialTimeInterval),
    _MaximumTimeout(InitialParameters._MaximumTimeout),
    _MinimumTimeout(InitialParameters._MinimumTimeout),
    _WeightOfExistingTime(InitialParameters._WeightOfExistingTime),
    _TimeToTimeoutMultipler(InitialParameters._TimeToTimeoutMultipler),
    _BackoffMultipler(InitialParameters._BackoffMultipler),
    _Tag(Tag)
/*++
 *
 * Routine Description:
 *      This function initializes the KAdaptiveTimerRoot object
 *
 * Arguments:
 *      InitialParameters - Supplies the Parameters that should be used for this
 *          timer.
 *
 *       Allocator - Supplies the allocator to use for memory allocations.
 *
 *       Tag - Supplies the allocation tag to use for memory allocations.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    UNREFERENCED_PARAMETER(Allocator);
}

NTSTATUS
KAdaptiveTimerRoot::AllocateTimerInstance(
    __out KSharedPtr<KAdaptiveTimerRoot::TimerInstance>& Timer
    )
/*++
 *
 * Routine Description:
 *      This function allocates and initializes an TimerInstance object
 *
 * Arguments:
 *      TimerInstanse - Returns the _new object if the allocation was successful.
 *
 * Return Value:
 *      Returns the status of the operation.
 *
 * Note:
 *
-*/
{
    Timer = _new(_Tag, GetThisAllocator()) TimerInstance( this );
    if (Timer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Timer->Status();
    if (!NT_SUCCESS(status))
    {
        Timer.Reset();     // Should do a release and cause destruction of failed instance
    }
    return status;
}

VOID
KAdaptiveTimerRoot::ApplyNewTime(
    ULONG TimeInterval
    )
/*++
 *
 * Routine Description:
 *      This routine incorperates the new time interval into the running estimate.
 *      The calculation is done with a 64 bit value.
 *
 * Arguments:
 *      TimeInterval - Supplies the next time interval
 *
 * Return Value:
 *      None.
 *
 * Note:
 *      This routine ignores races since the effect is to just drop one update.
-*/
{
    ULONGLONG Total = _CurrentTimeInterval;

    Total *= _WeightOfExistingTime;
    Total += ((ULONGLONG) TimeInterval * (Denominator - _WeightOfExistingTime));
    Total = Total/Denominator;

    _CurrentTimeInterval = (ULONG) Total;
}

ULONG
KAdaptiveTimerRoot::GetTimeout(
    ULONG PreviousTimeout
    )
/*++
 *
 * Routine Description:
 *      This routine returns  timeout that a timer instance should use.   It
 *      is based off the _CurrentTimeInterval or the PreviousTimeout
 *
 * Arguments:
 *      PreviousTimeout - Supplies the previously used timeout or zero if this is the
 *      first time.
 *
 * Return Value:
 *      Returns the initial timeout value in milliseconds.
 *
 * Note:
 *
-*/
{
    ULONG Timeout;

    Timeout = PreviousTimeout == 0 ? _CurrentTimeInterval * _TimeToTimeoutMultipler :
        PreviousTimeout * _BackoffMultipler;

    if (Timeout > _MaximumTimeout)
    {
        Timeout = _MaximumTimeout;
    }
    else if (Timeout <  _MinimumTimeout )
    {
        Timeout =  _MinimumTimeout;
    }

    return(Timeout);
}

KAdaptiveTimerRoot::~KAdaptiveTimerRoot() {};

KAdaptiveTimerRoot::TimerInstance::TimerInstance(
    __in KAdaptiveTimerRoot* TimerRoot
    ) :
    _TimerRoot(TimerRoot),
    _StartTime(0),
    _PreviousTimeout(0)
{
}

VOID
KAdaptiveTimerRoot::TimerInstance::StartTimer(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase::CompletionCallback Callback
    )
/*++
 *
 * Routine Description:
 *      This routine starts the timer at the begining of time interval.  The timer
 *      will use use a timeout value from the TimerRoot.
 *
 * Arguments:
 *      Parent - Passed to the KTimer start.
 *
 *      Callback - Passed to the KTimer start.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    ULONG Timeout = _TimerRoot->GetTimeout(_PreviousTimeout);

    _PreviousTimeout = Timeout;

    if (_StartTime == 0)
    {
        _StartTime = KNt::GetPerformanceTime();
    }

    KTimer::StartTimer( _PreviousTimeout, Parent, Callback );

}

VOID
KAdaptiveTimerRoot::TimerInstance::StopTimer()
/*++
 *
 * Routine Description:
 *      This routine stops the timer if it was operating and updates the root time
 *      interval.
 *
 * Arguments:
 *      None
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    LONGLONG TimeDelta;

    TimeDelta = InterlockedExchange64( &_StartTime, 0 );

    if (TimeDelta == 0)
    {
        //
        // The time interval was not started or has already been stopped. Note
        // originally there was a an assert here that timer status was not
        // STATUS_PENDING, but the assert would sometimes fire becuase the cancel
        // was still pending.
        //

        return;
    }

    //
    // Reset the _PreviousTimeout value.
    //

    _PreviousTimeout = 0;

    //
    // Calculate the time delta and convert to Milliseconds.
    //

    TimeDelta = (KNt::GetPerformanceTime() - TimeDelta ) / TicksPerMillisecond;
    KAssert( TimeDelta == (ULONG ) TimeDelta );

    _TimerRoot->ApplyNewTime((ULONG) TimeDelta);

    //
    // If the timer is still running cancel it now.
    //
    // Do this after this instance finishes all interactions with _TimerRoot.
    // By the time the timer callback fires, this instance will no longer
    // need the timer root. The user can safely call SetTimerRoot().
    //

    Cancel();
}

VOID
KAdaptiveTimerRoot::TimerInstance::SetTimerRoot(
    __in KAdaptiveTimerRoot::SPtr NewTimerRoot
    )
/*++
 *
 * Routine Description:
 *      This routine sets a new timer root for this instance.
 *      This timer instance should be in a reusable state.
 *
 * Arguments:
 *      NewTimerRoot - Supplies the new timer root.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    KAssert(Status() == K_STATUS_NOT_STARTED);
    _TimerRoot = Ktl::Move(NewTimerRoot);
}

KAdaptiveTimerRoot::TimerInstance::~TimerInstance() {}

VOID
KAdaptiveTimerRoot::Parameters::InitializeParameters()
/*++
 *
 * Routine Description:
 *      This routine initializes the AdaptiveTimer parameter class to a resonable set
 *      of default parameters.  It is an function rather than a constructor so the
 *      parameters struction can be a global in a driver where a constructor would
 *      not be allowed.
 *
 *      Becuase the timers are adpative the initial settings are not critical.
 *
 * Arguments:
 *      None.
 *
 * Return Value:
 *
 *
 * Note:
 *
-*/
{
    _MaximumTimeout = 10 * 1000; /* 10 Seconds*/
    _MinimumTimeout = 20;         /* 20 Milliseconds */
    _InitialTimeInterval = 250;  /* 250 Milliseconds */
    _WeightOfExistingTime = (7 * Denominator)/8;  /* 7/8 */
    _TimeToTimeoutMultipler = 4;
    _BackoffMultipler = 2;
}

VOID
KAdaptiveTimerRoot::Parameters::ModifyParameters(
    __in ULONG Count,
    __in_ecount(Count) LPCWSTR ModifyStrings[]
    )
/*++
 *
 * Routine Description:
 *      This function is used modify the parameters for an adaptive timer.  The
 *      changes are passed in as a set of Name, Value strings which are applied to
 *      parameters.  After the changes are applied a minimal sanity check is
 *      performed.
 *
 * Arguments:
 *      Count - Indicates the number of strings in being passed in. This should be
 *          even.
 *
 *      ModifyStrings - Pointer to an array of unicode strings pairs. The first
 *          string is the name of the parameter to modify and the second string is the
 *          new value to apply.
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/
{
    ULONG i;
    ULONG TempParameter;

    //
    // Loop through the name strings, skipping the values.
    //

    if (Count % 2)
    {
        KAssert((Count % 2) == 0 );

        //
        // Make sure there is always a i and i+1 string.
        //

        Count--;
    }

    for (i = 0; i < Count - 1; i += 2)
    {
        if (_wcsicmp( ModifyStrings[i], L"MaximumTimeout" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _MaximumTimeout = TempParameter;
            }

            continue;
        }

        if (_wcsicmp( ModifyStrings[i], L"MinimumTimeout" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _MinimumTimeout = TempParameter;
            }

            continue;
        }
        if (_wcsicmp( ModifyStrings[i], L"InitialTimeInterval" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _InitialTimeInterval = TempParameter;
            }

            continue;
        }
        if (_wcsicmp( ModifyStrings[i], L"WeightOfExistingTime" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _WeightOfExistingTime = TempParameter;
            }

            continue;
        }
        if (_wcsicmp( ModifyStrings[i], L"TimeToTimeoutMultipler" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _TimeToTimeoutMultipler = TempParameter;
            }

            continue;
        }
        if (_wcsicmp( ModifyStrings[i], L"BackoffMultipler" ) == 0)
        {
            TempParameter = _wtoi( ModifyStrings[i+1] );
            if (TempParameter != 0)
            {
                _BackoffMultipler = TempParameter;
            }

            continue;
        }
    }

    //
    // Preform a few sanity checks.  A parameter is never set to zero so those cases
    // are not an issue.  Check that MinimumTimeout is less than MaximumTimeout and
    // that WeightOfExistingTime is less than or equal to Denominator.
    //

    if (_MinimumTimeout > _MaximumTimeout)
    {
        KAssert(_MinimumTimeout <= _MaximumTimeout);
        _MaximumTimeout = _MinimumTimeout + 1;
    }

    if (_WeightOfExistingTime > KAdaptiveTimerRoot::Denominator )
    {
        KAssert(_WeightOfExistingTime <= KAdaptiveTimerRoot::Denominator);
        _WeightOfExistingTime = KAdaptiveTimerRoot::Denominator;
    }
}

#if defined(PLATFORM_UNIX)
#undef _wtoi
#endif
