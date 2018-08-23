/*++

Module Name:

    kevent.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KEvent object.

Author:

    Norbert P. Kusters (norbertk) 27-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#if defined(PLATFORM_UNIX)
#include <sys/time.h>
#include <kfinally.h>
#endif


KEvent::~KEvent(
    )
{
    Cleanup();
}

KEvent::KEvent(
    __in BOOLEAN IsManualReset,
    __in BOOLEAN InitialState
    )
{
    Zero();
    SetConstructorStatus(Initialize(IsManualReset, InitialState));
}

VOID
KEvent::SetEvent(
    )

/*++

Routine Description:

    This routine will set the event.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    ::SetEvent(_EventHandle);
#else
    pthread_mutex_lock(&_mutex);
    KFinally([this] { pthread_mutex_unlock(&_mutex); });
    _signaled = true;
    if (_manual)
    {
        pthread_cond_broadcast(&_cond);
        return;
    }
    pthread_cond_signal(&_cond);
#endif
#else
    KeSetEvent(&_Event, IO_NO_INCREMENT, FALSE);
#endif
}

VOID
KEvent::ResetEvent(
    )

/*++

Routine Description:

    This routine will reset the event.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    ::ResetEvent(_EventHandle);
#else
    pthread_mutex_lock(&_mutex);
    KFinally([this] { pthread_mutex_unlock(&_mutex); });
    _signaled = false;
#endif
#else
    KeResetEvent(&_Event);
#endif
}

BOOLEAN
KEvent::WaitUntilSet(
    __in_opt ULONG TimeoutInMilliseconds
    )

/*++

Routine Description:

    This routine waits until the event is in the signalled state.  If a timeout value is provided, then the call may complete
    with a return value of FALSE, indicating that the event is still not signalled but that the timeout has expired.

Arguments:

    TimeoutInMilliseconds   - Supplies, optionally, the timeout in milliseconds.

Return Value:

    FALSE   - The wait timed out.  The event is not signaled.

    TRUE    - The event is signalled.  As required.

--*/

{
#if KTL_USER_MODE

#if !defined(PLATFORM_UNIX)
    DWORD r;

    r = WaitForSingleObject(_EventHandle, TimeoutInMilliseconds);

    if (r == WAIT_TIMEOUT) {
        KFatal(TimeoutInMilliseconds != Infinite);
        return FALSE;
    }

    return TRUE;
#else
    timeval now = {};
    gettimeofday(&now, nullptr);
    timespec to =
            {
                .tv_sec = now.tv_sec + (now.tv_usec + TimeoutInMilliseconds * 1e3)/1e6,
                .tv_nsec = ((uint64_t)(now.tv_usec + TimeoutInMilliseconds * 1e3) % 1000000) * 1e3
            };
    pthread_mutex_lock(&_mutex);
    KFinally([this] { pthread_mutex_unlock(&_mutex); });
    while (!_signaled && !_closed)
    {
        int retval = 0;
        if (TimeoutInMilliseconds >= Infinite) {
            retval = pthread_cond_wait(&_cond, &_mutex);
        }
        else {
            retval = pthread_cond_timedwait(&_cond, &_mutex, &to);
        }

        if (retval == EINTR) continue;

        if (retval == ETIMEDOUT)
        {
            if (_signaled || _closed) break;
            return FALSE;
        }
    }
    if (!_manual) _signaled = false;
    return TRUE;
#endif
#else

    NTSTATUS status;
    LARGE_INTEGER* pTimeout;
    LARGE_INTEGER timeout;

    if (TimeoutInMilliseconds == Infinite) {
        pTimeout = NULL;
    } else {
        pTimeout = &timeout;
        timeout.QuadPart = (LONGLONG) (-10000)*TimeoutInMilliseconds;
    }

    status = KeWaitForSingleObject(&_Event, Executive, KernelMode, FALSE, pTimeout);

    if (status == STATUS_TIMEOUT) {
        KFatal(pTimeout);
        return FALSE;
    }

    return TRUE;

#endif
}

VOID
KEvent::Zero(
    )

/*++

Routine Description:

    This routine sets up default values on the semaphore.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    _EventHandle = NULL;
#endif
#endif
}

VOID
KEvent::Cleanup(
    )

/*++

Routine Description:

    This routine will cleanup an event object.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    if (_EventHandle) {
        CloseHandle(_EventHandle);
    }
#else
    if (_closed) return;
    pthread_mutex_lock(&_mutex);
    KFinally([this] { pthread_mutex_unlock(&_mutex); });
    _closed = true;
    _signaled = true;
    pthread_cond_broadcast(&_cond);
#endif
#endif
}

NTSTATUS
KEvent::Initialize(
    __in BOOLEAN IsManualReset,
    __in BOOLEAN InitialState
    )

/*++

Routine Description:

    This routine initializes a KEvent.

Arguments:

    IsManualReset   - Supplies whether or not this is a manual reset event.

    InitialState    - Supplies the event's initial state.

Return Value:

    NTSTATUS

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    _EventHandle = CreateEvent(NULL, IsManualReset, InitialState, NULL);
    if (!_EventHandle) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#else
    _manual = IsManualReset;
    _closed = false;
    _signaled = InitialState;
    pthread_cond_init(&_cond, nullptr);
    pthread_mutex_init(&_mutex, nullptr);
#endif
#else
    KeInitializeEvent(&_Event, IsManualReset ? NotificationEvent : SynchronizationEvent, InitialState);
#endif

    return STATUS_SUCCESS;
}
