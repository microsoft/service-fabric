/*++

Module Name:

    kspinlock.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KSpinLock object.

Author:

    Norbert P. Kusters (norbertk) 27-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>


KSpinLock::KSpinLock()
/*++

Routine Description:

    This routine initializes a spin lock object.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
    InitializeSRWLock(&_Lock);
#else
    KeInitializeSpinLock(&_SpinLock);
#endif
}

#if defined(PLATFORM_UNIX)
KSpinLock::~KSpinLock()
{
    // Need to explicitly destroy the lock on Linux
    pthread_rwlock_destroy((pthread_rwlock_t*)(&_Lock));
}
#endif

VOID
KSPINLOCK_SAL_ACQUIRE
KSpinLock::Acquire()

/*++

Routine Description:

    This routine acquires the spin lock.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
    AcquireSRWLockExclusive(&_Lock);
    _ThreadId = GetCurrentThreadId();
#else
    KeAcquireSpinLock(&_SpinLock, &_OldIrql);
    _ThreadId = KeGetCurrentThread();
#endif
}

#if KTL_USER_MODE
BOOLEAN KSPINLOCK_SAL_ACQUIRE
KSpinLock::TryAcquire()
{
    return TryAcquireSRWLockExclusive(&_Lock);
}
#endif

VOID
KSPINLOCK_SAL_RELEASE
KSpinLock::Release()

/*++

Routine Description:

    This routine releases the spin lock.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE
    _ThreadId = 0;
    ReleaseSRWLockExclusive(&_Lock);
#else
    _ThreadId = NULL;
    KeReleaseSpinLock(&_SpinLock, _OldIrql);
#endif
}

BOOLEAN
KSpinLock::IsOwned()

/*++

Routine Description:

    This routine will return whether or not the calling thread has already acquired the spin lock.

Arguments:

    None.

Return Value:

    FALSE   - The spin lock is not owned by this thread.

    TRUE    - The spin lock is owned by this thread.

--*/

{
    BOOLEAN r;

#if KTL_USER_MODE
    if (_ThreadId == GetCurrentThreadId()) {
#else
    if (_ThreadId == KeGetCurrentThread()) {
#endif
        r = TRUE;
    } else {
        r = FALSE;
    }

    return r;
}

KSTACKSPINLOCK_CTOR_SAL
KInStackSpinLock::KInStackSpinLock(
    __in KSpinLock& Lock
    )
{
    _Lock = &Lock;
    _Lock->Acquire();
    _HasLock = TRUE;
}

KSTACKSPINLOCK_DTOR_SAL
KInStackSpinLock::~KInStackSpinLock(
    )
{
    if (_HasLock != FALSE) {
        _HasLock = FALSE;        
        _Lock->Release();
    }
}

_Post_satisfies_(return == this->_HasLock)
BOOLEAN
KInStackSpinLock::HasLock(
    ) const
{
    return _HasLock;
}

VOID
KSTACKSPINLOCK_SAL_ACQUIRE
KInStackSpinLock::AcquireLock(
    )
{
    KFatal(!_HasLock);

    _Lock->Acquire();
    _HasLock = TRUE;
}

VOID
KSTACKSPINLOCK_SAL_RELEASE
KInStackSpinLock::ReleaseLock(
    )
{
    KFatal(_HasLock);

    _HasLock = FALSE;
    _Lock->Release();
}


#if KTL_USER_MODE
//** KReaderWriterSpinLock imp
KReaderWriterSpinLock::KReaderWriterSpinLock() noexcept
{
    InitializeSRWLock(&_Lock);
}

#if defined(PLATFORM_UNIX)
KReaderWriterSpinLock::~KReaderWriterSpinLock()
{
    // Need to explicitly destroy the lock on Linux
    pthread_rwlock_destroy((pthread_rwlock_t*)(&_Lock));
}
#endif

void KReaderWriterSpinLock::AcquireExclusive() noexcept
{
    AcquireSRWLockExclusive(&_Lock);
}

void KReaderWriterSpinLock::ReleaseExclusive() noexcept
{
    ReleaseSRWLockExclusive(&_Lock);
}

void KReaderWriterSpinLock::AcquireShared() noexcept
{
    AcquireSRWLockShared(&_Lock);
}

void KReaderWriterSpinLock::ReleaseShared() noexcept
{
    ReleaseSRWLockShared(&_Lock);
}

BOOLEAN KReaderWriterSpinLock::TryAcquireExclusive() noexcept
{
    return TryAcquireSRWLockExclusive(&_Lock);
}

BOOLEAN KReaderWriterSpinLock::TryAcquireShared() noexcept
{
    return TryAcquireSRWLockShared(&_Lock);
}
#endif

