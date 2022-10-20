// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/ScopedResourceOwner.h"

#ifdef LOCKTRACE

#define GENERIC_RWLOCK(lockNameTraits, lockVar) Common::RWLockWrapper<lockNameTraits> lockVar


#define LOCKNAME_TRAITS_FUNC(lockName)          static char const* GetLockName() { return #lockName; }

#define LOCKNAME_TRAITS(lockName, lockVar)      public: \
        struct lockVar##Traits \
        { \
            LOCKNAME_TRAITS_FUNC(lockName) \
        }; \
        private: \

#define RWLOCK(lockName, lockVar)               LOCKNAME_TRAITS(lockName, lockVar) \
                                                GENERIC_RWLOCK(lockVar##Traits, lockVar)

#define MUTABLE_RWLOCK(lockName, lockVar)       LOCKNAME_TRAITS(lockName, lockVar) \
                                                mutable GENERIC_RWLOCK(lockVar##Traits, lockVar)

#define STATIC_RWLOCK(lockName, lockVar)        LOCKNAME_TRAITS(lockName, lockVar) \
                                                static GENERIC_RWLOCK(lockVar##Traits, lockVar)

#define STATIC_RWLOCK_DEFINE(className, lockVar)    Common::RWLockWrapper<className::lockVar##Traits> className::lockVar

#define LOCKAPI                                 virtual void

#define LOCKNAME_TEMPLATE_TRAITS(traitName)     template <class T> struct traitName {  static char const* GetLockName(); };

#define LOCKNAME_TEMPLATE_TRAITS_LIST(traitName, className, lockName)    template <> struct traitName<className> {  LOCKNAME_TRAITS_FUNC(lockName); };

#else

#define GENERIC_RWLOCK(lockNameTraits, lockVar) Common::RwLock lockVar

#define LOCKNAME_TRAITS_FUNC(lockName)

#define RWLOCK(lockName, lockVar)               Common::RwLock lockVar
#define MUTABLE_RWLOCK(lockName, lockVar)       mutable Common::RwLock lockVar
#define STATIC_RWLOCK(lockName, lockVar)        static Common::RwLock lockVar
#define STATIC_RWLOCK_DEFINE(className, lockVar)    Common::RwLock className::lockVar

#define LOCKAPI                                 void

#define LOCKNAME_TEMPLATE_TRAITS(traitName)

#define LOCKNAME_TEMPLATE_TRAITS_LIST(traitName, className, lockName)

#endif  // LOCKTRACE

namespace Common
{
    class RWLockBase
    {
        DENY_COPY_ASSIGNMENT(RWLockBase)
    public:
        typedef AcquireSharedTraits<RWLockBase>        AcquireSharedTraitsT;
        typedef AcquireExclusiveTraits<RWLockBase>     AcquireExclusiveTraitsT;

#ifdef DBG
        RWLockBase() : owners_(), dtor_(false)
#else
        RWLockBase()
#endif
        { 
            InitializeSRWLock(&lock_); 
#ifdef DBG
            InitializeSRWLock(&ownerSetLock_);
#endif
        }

        // SRWLOCK can not be moved or copied.
        // We initialize it in copy and move constructor so that
        // we can allow "default" copy and move constructor for classes using RWLockBase.
        // This class(RWLockBase) should not be explictly copied or moved.
        RWLockBase(RWLockBase const &) : RWLockBase() {}

        RWLockBase(RWLockBase &&) : RWLockBase() {}

#ifdef DBG
        virtual ~RWLockBase()
        {
            dtor_ = true;
        }

#endif
        _Acquires_exclusive_lock_(lock_)
        LOCKAPI AcquireExclusive(); 

        _Releases_exclusive_lock_(lock_)
        LOCKAPI ReleaseExclusive();

        _Acquires_shared_lock_(lock_)
        LOCKAPI AcquireShared();

        _Releases_shared_lock_(lock_)
        LOCKAPI ReleaseShared();

    protected:

        friend class ConditionVariable;

        SRWLOCK lock_;

    private:

#ifdef DBG
        void CheckOwners();
        void AddOwner();
        void RemoveOwner();

        std::set<int> owners_;
        SRWLOCK ownerSetLock_;
        bool dtor_;
#else
    #define CheckOwners()
    #define AddOwner()
    #define RemoveOwner()
#endif
    };

    extern Common::StringLiteral const TraceAcquireExclusive;
    extern Common::StringLiteral const TraceReleaseExclusive;
    extern Common::StringLiteral const TraceAcquireShared;
    extern Common::StringLiteral const TraceReleaseShared;

    struct RwLockTracer
    {
        static void TraceLock(StringLiteral type, void* address, char const* lockName);
    };

    template<typename LockNameTraitsT>
    class RWLockWrapper : public RWLockBase
    {
        DENY_COPY(RWLockWrapper);

    public:
        RWLockWrapper() : RWLockBase() {}

        LOCKAPI AcquireExclusive()
        {
            RWLockBase::AcquireExclusive();
            RwLockTracer::TraceLock(TraceAcquireExclusive, &lock_, LockNameTraitsT::GetLockName());
        }

        LOCKAPI ReleaseExclusive()
        {
            RwLockTracer::TraceLock(TraceReleaseExclusive, &lock_, LockNameTraitsT::GetLockName());
            RWLockBase::ReleaseExclusive();
        }

        LOCKAPI AcquireShared()
        {
            RWLockBase::AcquireShared();
            RwLockTracer::TraceLock(TraceAcquireShared, &lock_, LockNameTraitsT::GetLockName());
        }

        LOCKAPI ReleaseShared()
        {
            RwLockTracer::TraceLock(TraceReleaseShared, &lock_, LockNameTraitsT::GetLockName());
            RWLockBase::ReleaseShared();
        }
    };

    typedef RWLockBase NonRecursiveRWLock;
    typedef NonRecursiveRWLock NonRecursiveExclusiveLock;

    typedef ScopedResOwner<NonRecursiveRWLock, NonRecursiveRWLock::AcquireSharedTraitsT> AcquireNonRecursiveSharedLock;
    typedef ScopedResOwner<NonRecursiveRWLock, NonRecursiveRWLock::AcquireExclusiveTraitsT> AcquireNonRecursiveExclusiveLock;

    typedef NonRecursiveRWLock RwLock;
    typedef RwLock ExclusiveLock;

    class AcquireReadLock
    {
    public:
        explicit AcquireReadLock(RwLock & rw) : owner_(&rw) {}
    private:
        AcquireNonRecursiveSharedLock owner_;
    };

    class AcquireWriteLock
    {
    public:
        explicit AcquireWriteLock(RwLock & rw) : owner_(&rw) {}
    private:
        AcquireNonRecursiveExclusiveLock owner_;
    };

    typedef AcquireWriteLock AcquireExclusiveLock;

#if !defined(PLATFORM_UNIX)
    class ConditionVariable
    {
        DENY_COPY(ConditionVariable);

    public:
        ConditionVariable(RwLock & lock)
            : lock_(lock)
        {
            InitializeConditionVariable(&conditionVariable_);
        }

        void Sleep(TimeSpan timeout)
        {
            SleepConditionVariableSRW(&conditionVariable_, &(lock_.lock_), timeout == TimeSpan::MaxValue ? INFINITE : static_cast<DWORD>(timeout.TotalMilliseconds()), 0);
        }

        void Wake()
        {
            WakeConditionVariable(&conditionVariable_);
        }

    private:
        RwLock & lock_;
        CONDITION_VARIABLE conditionVariable_;
    };
#else
    class Mutex
    {
        DENY_COPY(Mutex);

    public:

        Mutex()
        {
            pthread_mutex_init(&mutex_, NULL);
        };

        ~Mutex()
        {
            pthread_mutex_destroy(&mutex_);
        };

        LOCKAPI Acquire()
        {
            pthread_mutex_lock(&mutex_);
        }

        LOCKAPI Release()
        {
            pthread_mutex_unlock(&mutex_);
        }

        pthread_mutex_t* GetRawPointer()
        {
            return &mutex_;
        }

    private:
        pthread_mutex_t mutex_;
    };

#define MUTABLE_MUTEX(lockName, lockVar)   mutable Common::Mutex lockVar

    typedef ScopedResOwner<Mutex> AcquireMutexLock;

    class ConditionVariable
    {
    DENY_COPY(ConditionVariable);

    public:
        ConditionVariable(Mutex& lock)
                : lock_(&lock)
        {
            pthread_cond_init(&condition_, NULL);
        }

        void Sleep(TimeSpan timeout)
        {
            int64 nanoseconds = timeout.get_Ticks() * (1000000 / TimeSpan::TicksPerMillisecond);

            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += nanoseconds / 1000000000;
            ts.tv_nsec += nanoseconds % 1000000000;
            if (ts.tv_nsec > 1000000000) {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec++;
            }

            pthread_cond_timedwait(&condition_, lock_->GetRawPointer(), &ts);
        }

        void Wake()
        {
            pthread_cond_signal(&condition_);
        }

    private:
        Mutex* lock_;
        pthread_cond_t condition_;
    };

#endif
} //namespace Common

