/*++

Module Name:

    ksemaphore.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KSemaphore object.

Author:

    Norbert P. Kusters (norbertk) 27-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#if defined(PLATFORM_UNIX)
#include <semaphore.h>
#endif


KSemaphore::~KSemaphore(
    )
{
    Cleanup();
}

KSemaphore::KSemaphore(
    __in LONG InitialCount,
    __in LONG MaximumCount
    )
{
    Zero();
    SetConstructorStatus(Initialize(InitialCount, MaximumCount));
}

VOID
KSemaphore::Subtract(
    )

/*++

Routine Description:

    This routine will wait for the semaphore's count to be at least one and then subtract one from that count.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    WaitForSingleObject(_SemaphoreHandle, INFINITE);
#else
    int ret = 0;
    do {
        ret = sem_wait(&_Semaphore);
    } while (ret != 0 && errno == EINTR);
#endif
#else
    KeWaitForSingleObject(&_Semaphore, Executive, KernelMode, FALSE, NULL);
#endif
}

VOID
KSemaphore::Add(
    __in ULONG Count
    )

/*++

Routine Description:

    This routine adds the given count to the semaphore count.

Arguments:

    Count   - Supplies the count.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE

#if !defined(PLATFORM_UNIX)
    BOOL b = ReleaseSemaphore(_SemaphoreHandle, Count, NULL);
#else
    BOOL b = !sem_post(&_Semaphore);
#endif

    KFatal(b);

#else
    KeReleaseSemaphore(&_Semaphore, IO_NO_INCREMENT, Count, FALSE);
#endif
}

VOID
KSemaphore::Zero(
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
    _SemaphoreHandle = NULL;
#endif
#endif
}

VOID
KSemaphore::Cleanup(
    )

/*++

Routine Description:

    This routine will cleanup a semaphore object.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    if (_SemaphoreHandle) {
        CloseHandle(_SemaphoreHandle);
    }
#else
        sem_destroy(&_Semaphore);
#endif
#endif
}

NTSTATUS
KSemaphore::Initialize(
    __in LONG InitialCount,
    __in LONG MaximumCount
    )

/*++

Routine Description:

    This routine initializes a KSemaphore.

Arguments:

    InitialCount    - Supplies the initial count.

    MaximumCount    - Supplies the maximum count.

Return Value:

    NTSTATUS

--*/

{
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
    _SemaphoreHandle = CreateSemaphore(NULL, InitialCount, MaximumCount, NULL);
    if (!_SemaphoreHandle) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#else
    if (0 != sem_init(&_Semaphore, 0, InitialCount))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif
#else
    KeInitializeSemaphore(&_Semaphore, InitialCount, MaximumCount);
#endif

    return STATUS_SUCCESS;
}
