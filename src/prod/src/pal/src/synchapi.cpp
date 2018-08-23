// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "synchapi.h"
#include <pthread.h>

#define RTLP_RUN_ONCE_START         0x00UL
#define RTLP_RUN_ONCE_INITIALIZING  0x01UL
#define RTLP_RUN_ONCE_COMPLETE      0x02UL
#define RTLP_RUN_ONCE_NONBLOCKING   0x03UL

WINBASEAPI
BOOL
WINAPI
InitOnceExecuteOnce(
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID * Context
    )
{
    while(TRUE)
    {
        if (InitOnce->State & RTLP_RUN_ONCE_COMPLETE)
        {
            return TRUE;
        }
        else if (InitOnce->State & RTLP_RUN_ONCE_INITIALIZING
             || InterlockedCompareExchange((volatile LONG*) &(InitOnce->State), RTLP_RUN_ONCE_INITIALIZING, 0) != 0)
        {
            Sleep(10); continue;
        }
        break;
    }

    BOOL success = InitFn(InitOnce, Parameter, Context);
    InitOnce->State = success ? RTLP_RUN_ONCE_COMPLETE : 0;
    return success;
}

WINBASEAPI
VOID
WINAPI
InitializeSRWLock(
    _Out_ PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    *lock = PTHREAD_RWLOCK_INITIALIZER;
    int result = pthread_rwlock_init(lock, NULL);
}


WINBASEAPI
_Releases_exclusive_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_unlock(lock);
}


WINBASEAPI
_Releases_shared_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_unlock(lock);
}


WINBASEAPI
_Acquires_exclusive_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_wrlock(lock);
}


WINBASEAPI
_Acquires_shared_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_rdlock(lock);
}

