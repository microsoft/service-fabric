/*++

Module Name:

    kthread.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KThread object.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#if defined(PLATFORM_UNIX)
#include <signal.h>
#include <unistd.h>
#define NULL 0
#endif


KThread::~KThread()
{
    Cleanup();
}

KThread::KThread(
    __in StartRoutine StartRoutine,
    __inout_opt VOID* Parameter
    )
{
    Zero();
    SetConstructorStatus(Initialize(StartRoutine, Parameter));
}

NTSTATUS
KThread::Create(
    __in StartRoutine StartRoutine,
    __inout VOID* Parameter,
    __out KThread::SPtr& Thread,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new thread with the given start routine and returns a smart pointer to it.

Arguments:

    StartRoutine    - Supplies the start routine.

    Parameter       - Supplies the parameter.

    Thread          - Returns a smart pointer to the newly created thread.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    KThread* thread;
    NTSTATUS status;

    thread = _new(AllocationTag, Allocator) KThread(StartRoutine, Parameter);

    if (!thread) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = thread->Status();

    if (!NT_SUCCESS(status)) {
        _delete(thread);
        return status;
    }

    Thread = thread;

    return STATUS_SUCCESS;
}

VOID
KThread::Wait(
    )

/*++

Routine Description:

    This routine will wait until the thread exits.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE

    #if !defined(PLATFORM_UNIX)
    WaitForSingleObject(_ThreadHandle, INFINITE);
    #else
    VOID PALAPI WaitThread(HANDLE hThread);
    WaitThread(_ThreadHandle);
    #endif

#else

    KeWaitForSingleObject(&_ThreadStarted, Executive, KernelMode, FALSE, NULL);
    KeWaitForSingleObject(_Thread, Executive, KernelMode, FALSE, NULL);

#endif

    _WaitPerformed = TRUE;
}

VOID
KThread::Zero(
    )

/*++

Routine Description:

    This routine initializes the KThread object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _ThreadHandle = NULL;
    _StartRoutine = NULL;
    _Parameter = NULL;
    _WaitPerformed = FALSE;

#if KTL_USER_MODE
#else
    _Thread = NULL;
    KeInitializeEvent(&_ThreadStarted, NotificationEvent, FALSE);
#endif
}

VOID
KThread::Cleanup(
    )

/*++

Routine Description:

    This routine runs down the given thread, holding this thread hostage until it exits.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (!_ThreadHandle) {
        return;
    }

    if (!_WaitPerformed) {
        Wait();
    }

#if KTL_USER_MODE

    CloseHandle(_ThreadHandle);

#else

    KNt::Close(_ThreadHandle);

#endif
}


#if KTL_USER_MODE

DWORD
KThread::HostStartRoutine(
    __inout VOID* Thread
    )

/*++

Routine Description:

    This routine is the proxy start routine for this thread object.

Arguments:

    Thread  - Supplies a pointer to the thread object.

Return Value:

    None.

--*/

{
    KThread* thread = (KThread*) Thread;

    thread->_StartRoutine(thread->_Parameter);

    return 0;
}

#else

VOID
KThread::HostStartRoutine(
    __inout VOID* Thread
    )

/*++

Routine Description:

    This routine is the proxy start routine for this thread object.

Arguments:

    Thread  - Supplies a pointer to the thread object.

Return Value:

    None.

--*/

{
    KThread* thread = (KThread*) Thread;

    thread->_Thread = KeGetCurrentThread();
    KeSetEvent(&thread->_ThreadStarted, IO_NO_INCREMENT, FALSE);

    thread->_StartRoutine(thread->_Parameter);

    PsTerminateSystemThread(STATUS_SUCCESS);
}

#endif

NTSTATUS
KThread::Initialize(
    __in StartRoutine StartRoutine,
    __inout VOID* Parameter
    )

/*++

Routine Description:

    This routine initializes a new KThread object.

Arguments:

    StartRoutine    - Supplies the start routine.

    Parameter       - Supplies the parameter.

Return Value:

    NTSTATUS

--*/

{
    _StartRoutine = StartRoutine;
    _Parameter = Parameter;

#if KTL_USER_MODE

    _ThreadHandle = CreateThread(NULL, 0, KThread::HostStartRoutine, this, 0, NULL);

    if (!_ThreadHandle) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#else

    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = PsCreateSystemThread(&_ThreadHandle, 0, &oa, 0, NULL, KThread::HostStartRoutine, this);

    if (!NT_SUCCESS(status)) {
        _ThreadHandle = NULL;
        return status;
    }

#endif

    return STATUS_SUCCESS;
}


KThread::Id
KThread::GetCurrentThreadId()
//
//  Return the calling thread's unique ThreadId
//
{
    #if KTL_USER_MODE
        return static_cast<KThread::Id>(::GetCurrentThreadId());
    #else
        return reinterpret_cast<KThread::Id>(KeGetCurrentThread());
    #endif
}

const KThread::Id
KThread::InvalidThreadId = MAXULONGLONG;

BOOLEAN
KThread::SetThreadPriority(
    __in KThread::Priority Priority
    )
{
#if KTL_USER_MODE
    BOOL b = ::SetThreadPriority(_ThreadHandle, Priority);
    return b ? TRUE : FALSE;
#else
    KeSetPriorityThread(_Thread, Priority);
    return TRUE;
#endif
}
