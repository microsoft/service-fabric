/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    ksemaphore.h

Abstract:

    This file defines a semaphore class.  This class can be compiled for user mode also.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KSemaphore : public KObject<KSemaphore> {

    public:

        ~KSemaphore();

        FAILABLE
        KSemaphore(
            __in LONG InitialCount = 0,
            __in LONG MaximumCount = MAXLONG
            );

        VOID
        Subtract(
            );

        VOID
        Add(
            __in ULONG Count = 1
            );

    private:

        K_DENY_COPY(KSemaphore);

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in LONG InitialCount,
            __in LONG MaximumCount
            );

#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
        HANDLE _SemaphoreHandle;
#else
        sem_t _Semaphore;
#endif
#else
        KSEMAPHORE _Semaphore;
#endif

};
