/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kspinlock.h

Abstract:

    This file defines a spin lock class.  This class can be compiled for user mode also.  In this case the implementation
    uses a SRWLOCK.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

#pragma once

#if KTL_USER_MODE
#define KSPINLOCK_SAL_ACQUIRE \
        _Requires_lock_not_held_(this->_Lock) _Acquires_lock_(this->_Lock)
#else
#define KSPINLOCK_SAL_ACQUIRE \
        _Requires_lock_not_held_(this->_SpinLock) _Acquires_lock_(this->_SpinLock) \
        __drv_maxIRQL(DISPATCH_LEVEL) __drv_raisesIRQL(DISPATCH_LEVEL) _At_(this->_OldIrql, __drv_savesIRQL)
#endif

#if KTL_USER_MODE
#define KSPINLOCK_SAL_RELEASE \
        _Requires_lock_held_(this->_Lock) _Releases_lock_(this->_Lock)
#else
#define KSPINLOCK_SAL_RELEASE \
        _Requires_lock_held_(this->_SpinLock) _Releases_lock_(this->_SpinLock) \
        __drv_requiresIRQL(DISPATCH_LEVEL) _At_(this->_OldIrql, __drv_restoresIRQL)
#endif

#if defined(PLATFORM_UNIX)
#define KSTACKSPINLOCK_SAL_ACQUIRE
#elif KTL_USER_MODE
#define KSTACKSPINLOCK_SAL_ACQUIRE \
        _Requires_lock_not_held_(this->_Lock->_Lock) _Pre_satisfies_(this->_HasLock == FALSE) _Acquires_lock_(this->_Lock->_Lock) _Post_satisfies_(this->_HasLock == TRUE)
#else
#define KSTACKSPINLOCK_SAL_ACQUIRE \
        _Requires_lock_not_held_(this->_Lock->_SpinLock) _Pre_satisfies_(this->_HasLock == FALSE) _Acquires_lock_(this->_Lock->_SpinLock) _Post_satisfies_(this->_HasLock == TRUE) \
        __drv_maxIRQL(DISPATCH_LEVEL) __drv_raisesIRQL(DISPATCH_LEVEL) _At_(this->_Lock->_OldIrql, __drv_savesIRQL)
#endif

#if defined(PLATFORM_UNIX)
#define KSTACKSPINLOCK_CTOR_SAL
#elif KTL_USER_MODE
#define KSTACKSPINLOCK_CTOR_SAL \
        _Requires_lock_not_held_(this->_Lock->_Lock) _Acquires_lock_(this->_Lock->_Lock) _Post_satisfies_(this->_HasLock == TRUE) \
        _Post_same_lock_(Lock._Lock, this->_Lock->_Lock)
#else
#define KSTACKSPINLOCK_CTOR_SAL \
        _Requires_lock_not_held_(this->_Lock->_SpinLock) _Acquires_lock_(this->_Lock->_SpinLock) _Post_satisfies_(this->_HasLock == TRUE) \
        _Post_same_lock_(Lock._SpinLock, this->_Lock->_SpinLock)  \
        __drv_maxIRQL(DISPATCH_LEVEL) __drv_raisesIRQL(DISPATCH_LEVEL) _At_(this->_Lock->_OldIrql, __drv_savesIRQL)
#endif

#if defined(PLAFORM_UNIX)
#define KSTACKSPINLOCK_SAL_RELEASE
#elif KTL_USER_MODE
#define KSTACKSPINLOCK_SAL_RELEASE \
        _Requires_lock_held_(this->_Lock->_Lock) _Pre_satisfies_(this->_HasLock == TRUE) _Releases_lock_(this->_Lock->_Lock) _Post_satisfies_(this->_HasLock == FALSE)
#else
#define KSTACKSPINLOCK_SAL_RELEASE \
        _Requires_lock_held_(this->_Lock->_SpinLock) _Pre_satisfies_(this->_HasLock == TRUE) _Releases_lock_(this->_Lock->_SpinLock) _Post_satisfies_(this->_HasLock == FALSE) \
        __drv_requiresIRQL(DISPATCH_LEVEL) _At_(this->_Lock->_OldIrql, __drv_restoresIRQL)
#endif

#if defined(PLAFORM_UNIX)
#define KSTACKSPINLOCK_DTOR_SAL
#elif KTL_USER_MODE
#define KSTACKSPINLOCK_DTOR_SAL \
        _When_(this->_HasLock != FALSE, _Requires_lock_held_(this->_Lock->_Lock) _Releases_lock_(this->_Lock->_Lock) _Post_satisfies_(this->_HasLock == FALSE))
#else
#define KSTACKSPINLOCK_DTOR_SAL \
        _When_(this->_HasLock != FALSE, \
            _Requires_lock_held_(this->_Lock->_SpinLock) _Releases_lock_(this->_Lock->_SpinLock) _Post_satisfies_(this->_HasLock == FALSE) \
            __drv_requiresIRQL(DISPATCH_LEVEL) _At_(this->_Lock->_OldIrql, __drv_restoresIRQL))
#endif

class KSpinLock {

    K_DENY_COPY(KSpinLock);

    public:

        NOFAIL KSpinLock();
#if defined(PLATFORM_UNIX)
        // need to destroy the lock explicity on Linux
        ~KSpinLock();
#endif

        VOID
        KSPINLOCK_SAL_ACQUIRE
        Acquire();

        #if KTL_USER_MODE
            BOOLEAN 
            KSPINLOCK_SAL_ACQUIRE 
            TryAcquire();
        #endif

        VOID
        KSPINLOCK_SAL_RELEASE
        Release();

        BOOLEAN
        IsOwned();

    private:

#if KTL_USER_MODE
        SRWLOCK _Lock;
        DWORD _ThreadId;
#else
        KSPIN_LOCK _SpinLock;
        KIRQL _OldIrql;
        PKTHREAD _ThreadId;
#endif

};

//
// Struct that defines storage for a KSpinLock that is defined as a
// static global. KTL static globals may not have constructors. Luckily
// in the case of a KSpinLock (and underlying KSPIN_LOCK and SRWLOCK)
// that initialization the initialization to all zeros that the C
// compiler provides is sufficient.
//
struct KGlobalSpinLockStorage {
    inline KSpinLock& Lock() { return (*((KSpinLock*)&_Storage)); };
    UCHAR _Storage[sizeof(KSpinLock)];
};

//
// A helper class to use KSpinLock within a stack-storage scope.
// It acquires the lock in ctor and releases the lock in dtor.
//
// It offers two advantages over plain KSpinLock:
// (1) Safety. It is not possible not to release a spin lock when exiting a stack scope.
// (2) Convenience with automatic lock release semantic.
//

class KInStackSpinLock {

    K_DENY_COPY(KInStackSpinLock);
    K_NO_DYNAMIC_ALLOCATE();

    public:

        NOFAIL
        KSTACKSPINLOCK_CTOR_SAL
        KInStackSpinLock(
            __in KSpinLock& Lock
            );

        KSTACKSPINLOCK_DTOR_SAL
       ~KInStackSpinLock(
            );

        _Post_satisfies_(return == this->_HasLock)
        BOOLEAN
        HasLock(
            ) const;

        VOID
        KSTACKSPINLOCK_SAL_ACQUIRE
        AcquireLock(
            );

        VOID
        KSTACKSPINLOCK_SAL_RELEASE
        ReleaseLock(
            );

    private:

        //
        // Disable the default ctor.
        // KInStackSpinLock must be constructed from an existing KSpinLock.
        //

        KInStackSpinLock(
            );

        KSpinLock* _Lock;
        BOOLEAN _HasLock;

};

//
// This macro allows the use of a KSpinLock more visually recognizable.
// It constructs a local hidden object and use a for() loop.
//
// Note: Do not manually release lock on the hidden object.
// Use the "break" keyword to prematurely exit the block.
//
// Example:
//  KSpinLock spinLock;
//  ...
//
//  void MyFunction()
//  {
//      K_LOCK_BLOCK(spinLock)
//      {
//          // This block is protected by spinLock
//          ...
//      }
//      ...
//      K_LOCK_BLOCK(spinLock)
//      {
//          // This block is protected by spinLock
//          ...
//          break;  // Prematurely exit this K_LOCK_BLOCK
//      }
//      ...
//  }
//

#define __K_LOCK_BLOCK_VARIABLE_EXPAND(VariableName, Lock) \
    for (KInStackSpinLock VariableName(Lock); VariableName.HasLock(); VariableName.ReleaseLock())

//
// Prefast has trouble recognizing the for() loop.
//
// Suppress Prefast warning 28167 (This function <function> changes the IRQL
// and does not restore the IRQL before it exits.
// It should be annotated to reflect the change, or the IRQL should be restored).
//
// IRQL is restored when the lock is released.
// This issue is filed as BUG: 422165 in DevDiv TFS: Prefast not recognizing c++ dtor being called.
// Area Path is DevDiv\VS\WinC++\ALM Tools\Prefast.
//

#define K_LOCK_BLOCK(Lock) \
    __pragma(prefast(suppress: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called")) \
    __K_LOCK_BLOCK_VARIABLE_EXPAND(K_MAKE_UNIQUE_NAME(_localHiddenLock),Lock)
    

//
// K_LOCK_BLOCK_EX allows the user to specify the name of the local control lock so
// that acquire and release methods can be used.
//

#define K_LOCK_BLOCK_EX(Lock, Control) \
    __pragma(prefast(suppress: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called")) \
    for (KInStackSpinLock Control (Lock); (Control).HasLock(); (Control).ReleaseLock())


//** Reader/Writer spin lock
//
//   API:
//      AcquireExclusive() -- allow only one own at a time
//      ReleaseExclusive() -- drop single owner
//      AcquireShared()    -- Acquire access with other shared acquirers; increase shared lock help count
//      ReleaseShared()    -- drop shared lock count --- allowing threads held at AcquireExclusive() if last release
//      TryAcquireExclusive()   -- Same as AcquireExclusive() if TRUE is returned; else lock not taken
//      TryAcquireShared()      -- Same as AcquireShared() if TRUE is returned; else lock not taken
//
#if KTL_USER_MODE
class KReaderWriterSpinLock
{
    K_DENY_COPY(KReaderWriterSpinLock);

public:
    NOFAIL KReaderWriterSpinLock() noexcept;
#if defined(PLATFORM_UNIX)
    // Need to destroy the lock explicitly on Linux
    ~KReaderWriterSpinLock();
#endif

    void KSPINLOCK_SAL_ACQUIRE AcquireExclusive() noexcept;
    void KSPINLOCK_SAL_RELEASE ReleaseExclusive() noexcept;
    void KSPINLOCK_SAL_ACQUIRE AcquireShared() noexcept;
    void KSPINLOCK_SAL_RELEASE ReleaseShared() noexcept;

    BOOLEAN KSPINLOCK_SAL_ACQUIRE TryAcquireExclusive() noexcept;
    BOOLEAN KSPINLOCK_SAL_ACQUIRE TryAcquireShared() noexcept;

private:
    SRWLOCK         _Lock;
};

#endif
