/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kthread.h

Abstract:

    This file defines a thread class.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KThread : public KObject<KThread>, public KShared<KThread> {

    K_FORCE_SHARED(KThread);

    public:

        typedef
        VOID
        (*StartRoutine)(
            __inout_opt VOID* Parameter
            );

        static
        NTSTATUS
        Create(
            __in StartRoutine StartRoutine,
            __inout VOID* Parameter,
            __out KThread::SPtr& Thread,
            __in_opt KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_THREAD
            );

        VOID
        Wait(
            );

        typedef ULONGLONG Id;

        static KThread::Id
        GetCurrentThreadId();

        static const KThread::Id InvalidThreadId;

        //
        // In user-mode, see SetThreadPriority() for the definition of Priority.
        // In kernel-mode, see KeSetPriorityThread() for the definition of Priority.
        //

        typedef LONG Priority;

        BOOLEAN
        SetThreadPriority(
            __in KThread::Priority Priority
            );

    private:
        
        FAILABLE
        KThread(
            __in StartRoutine StartRoutine,
            __inout_opt VOID* Parameter
            );

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in StartRoutine StartRoutine,
            __inout VOID* Parameter
            );

#if KTL_USER_MODE
        static
        DWORD
        HostStartRoutine(
            __inout VOID* Thread
            );
#else
        static
        VOID
        HostStartRoutine(
            __inout VOID* Thread
            );
#endif

        HANDLE _ThreadHandle;
        StartRoutine _StartRoutine;
        VOID* _Parameter;
        BOOLEAN _WaitPerformed;

#if KTL_USER_MODE
#else
        PKTHREAD _Thread;
        KEVENT _ThreadStarted;
#endif

};
