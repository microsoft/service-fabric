#pragma once

#include <list>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include "Interlock.h"
#include "Volatile.h"

using namespace std;
namespace KtlThreadpool {

    class AutoResetEvent
    {
    public:
        explicit AutoResetEvent(bool initial = false) : flag_(initial)
        {
            pthread_mutex_init(&mutex_, NULL);
            pthread_cond_init(&condition_, NULL);
        }

        ~AutoResetEvent()
        {
            pthread_mutex_destroy(&mutex_);
            pthread_cond_destroy(&condition_);
        }

        inline void Set()
        {
            pthread_mutex_lock(&mutex_);
            flag_ = true;
            pthread_mutex_unlock(&mutex_);
            pthread_cond_signal(&condition_);
        }

        inline void Reset()
        {
            pthread_mutex_lock(&mutex_);
            flag_ = false;
            pthread_mutex_unlock(&mutex_);
        }

        inline bool WaitOne()
        {
            pthread_mutex_lock(&mutex_);
            while( !flag_ ) // prevent spurious wakeups from doing harm
                pthread_cond_wait(&condition_, &mutex_);
            flag_ = false; // waiting resets the flag
            pthread_mutex_unlock(&mutex_);
            return true;
        }

        inline int WaitOne(unsigned int milliseconds)
        {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += milliseconds / 1000;
            ts.tv_nsec += (milliseconds % 1000) * 1000000;
            if (ts.tv_nsec > 1000000000) {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }

            int rc = 0;
            pthread_mutex_lock(&mutex_);

            while (!flag_ && rc == 0) {
                rc = pthread_cond_timedwait(&condition_, &mutex_, &ts);
            }
            if (rc == 0)
                flag_ = false;

            pthread_mutex_unlock(&mutex_);
            return rc;
        }

    private:
        AutoResetEvent(const AutoResetEvent&);
        AutoResetEvent& operator=(const AutoResetEvent&); // non-copyable
        bool flag_;
        pthread_mutex_t mutex_;
        pthread_cond_t condition_;
    };

    class Semaphore {
    public:
        Semaphore(): initialized_(false) { }

        void Create(uint initialValue)
        {
            int ret = sem_init(&semaphore_, 0, initialValue);
            initialized_ = true;
        }

        ~Semaphore()
        {
            if (initialized_)
                sem_destroy(&semaphore_);
        }

        void Close();

        inline void Wait()
        {
            int ret = 0;
            do {
                ret = sem_wait(&semaphore_);
            } while (ret != 0 && errno == EINTR);
        }

        inline bool TryWait()
        {
            int ret = 0;
            do {
                ret = sem_trywait(&semaphore_);
            } while (ret != 0 && errno == EINTR);

            return ret == 0;
        }

        inline bool Wait(unsigned int milliseconds)
        {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += milliseconds / 1000;
            ts.tv_nsec += (milliseconds % 1000) * 1000000;
            if (ts.tv_nsec > 1000000000) {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }
            while (true) {
                int result = sem_timedwait(&semaphore_, &ts);
                if (result == 0) {
                    return true;
                } else if (errno == EINTR) {
                    continue;
                } else if (errno == ETIMEDOUT) {
                    return false;
                } else {
                    return false;
                }
            }
        }

        inline bool Release(uint count)
        {
            for (uint i = 0; i < count; i++) {
                int ret = sem_post(&semaphore_);
                if (ret != 0)
                    return false;
            }
            return true;
        }

    private:
        bool initialized_;
        sem_t semaphore_;
    };

    class CriticalSection
    {
    private:
        inline void Enter() { pthread_mutex_lock(&mutex_); }
        inline void Leave() { pthread_mutex_unlock (&mutex_); }

        pthread_mutex_t mutex_;

    public:
        CriticalSection()
        {
            pthread_mutexattr_t mAttr;
            pthread_mutexattr_init(&mAttr);
            pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);
            pthread_mutex_init(&mutex_, &mAttr);
        }

    public:
        static inline void AcquireLock(CriticalSection *c) { c->Enter(); }
        static inline void ReleaseLock(CriticalSection *c) { c->Leave(); }
    };

    class CriticalSectionHolder
    {
        CriticalSection *cs_;

    public:
        inline CriticalSectionHolder(CriticalSection *cs) : cs_(cs)
        {
            CriticalSection::AcquireLock(cs_);
        }

        inline ~CriticalSectionHolder()
        {
            CriticalSection::ReleaseLock(cs_);
        }
    };

    // Holder for the case where the acquire function can fail.
    template <typename VALUE, BOOL (*ACQUIRE)(VALUE value), void (*RELEASEF)(VALUE value)>
    class ConditionalStateHolder {
    private:
        VALUE m_value;
        BOOL m_acquired;      // Have we acquired the state?

    public:
        inline ConditionalStateHolder(VALUE value, BOOL take = TRUE) : m_value(value), m_acquired(FALSE)
        {
            if (take)
                Acquire();
        }

        inline ~ConditionalStateHolder()
        {
            Release();
        }

        inline BOOL Acquire()
        {
            _ASSERTE(!m_acquired);
            m_acquired = ACQUIRE(m_value);
            return m_acquired;
        }

        inline void Release()
        {
            if (m_acquired)
                RELEASEF(m_value);
            m_acquired = FALSE;
        }

        inline void Clear()
        {
            if (m_acquired)
                RELEASEF(m_value);
            m_acquired = FALSE;
        }

        inline void SuppressRelease()
        {
            m_acquired = FALSE;
        }
        inline BOOL Acquired()
        {
            return m_acquired;
        }
    };

    class DangerousSpinLock
    {
    public:
        DangerousSpinLock() { m_value = 0; }

    private:
        DangerousSpinLock(DangerousSpinLock const & other) = delete;

        inline void Acquire()
        {
            DWORD backoffs = 0;
            while (InterlockedExchange(&m_value, 1) == 1)
            {
                ++backoffs;
                __SwitchToThread(0, backoffs);
            }
        }

        inline BOOL TryAcquire()
        {
            return (InterlockedExchange(&m_value, 1) == 0);
        }

        inline void Release()
        {
            m_value = 0;
        }

        inline static void AcquireLock(DangerousSpinLock *pLock) { pLock->Acquire(); }
        inline static BOOL TryAcquireLock(DangerousSpinLock *pLock) { return pLock->TryAcquire(); }
        inline static void ReleaseLock(DangerousSpinLock *pLock) { pLock->Release(); }

        Volatile<LONG> m_value;

    public:
        class Holder
        {
            DangerousSpinLock * m_pDangerousSpinLock;

        public:
            Holder(DangerousSpinLock *s) : m_pDangerousSpinLock(s)
            {
                m_pDangerousSpinLock->Acquire();
            }

            ~Holder()
            {
                m_pDangerousSpinLock->Release();
            }
        };

    public:
        inline BOOL IsHeld() { return (BOOL)m_value; }

        typedef ConditionalStateHolder<DangerousSpinLock *,
                                       DangerousSpinLock::TryAcquireLock,
                                       DangerousSpinLock::ReleaseLock> TryHolder;
    };

    class SpinLock {
    private:

        enum SpinLockState {
            UnInitialized,
            BeingInitialized,
            Initialized
        };

        LONG m_lock;
        Volatile<SpinLockState> m_Initialized;

    public:
        SpinLock() { m_Initialized = UnInitialized; }

        ~SpinLock() { }

        inline void Init()
        {
            if (m_Initialized == Initialized)
                return;

            while (true)
            {
                LONG curValue = InterlockedCompareExchange((LONG*)&m_Initialized, BeingInitialized, UnInitialized);
                if (curValue == Initialized)
                {
                    return;
                }
                else if (curValue == UnInitialized)
                {
                    // We are the first to initialize the lock
                    break;
                }
                else
                {
                    __SwitchToThread(5, 0);
                }
            }
            m_lock = 0;
            _ASSERTE (m_Initialized == BeingInitialized);
            m_Initialized = Initialized;
        }

    private:
        inline void SpinToAcquire ()
        {
            DWORD backoffs = 0;
            ULONG ulSpins = 0;
            while (true)
            {
                for (unsigned i = ulSpins + 1000; ulSpins < i; ulSpins++)
                {
                    // Note: Must use Volatile to ensure the lock is refetched from memory.
                    if (VolatileLoad(&m_lock) == 0)
                        break;
                    YieldProcessor();                   // indicate to the processor that we are spining
                }

                // Try the inline atomic test again.
                if (GetLockNoWait())
                {
                    break;
                }
                backoffs++;
                __SwitchToThread(0, backoffs);
            }
        }

    private:
        inline void GetLock()
        {
            if (!GetLockNoWait())
            {
                SpinToAcquire();
            }
        }

        inline BOOL GetLockNoWait()
        {
            if (VolatileLoad(&m_lock) == 0 && InterlockedExchange(&m_lock, 1) == 0)
            {
                return 1;
            }
            return 0;
        }

        inline void FreeLock()
        {
            VolatileStore(&m_lock, (LONG)0);
        }

    public:
        static inline void AcquireLock(SpinLock *s) { s->GetLock(); }
        static inline void ReleaseLock(SpinLock *s) { s->FreeLock(); }

        class Holder
        {
            SpinLock *m_pSpinLock;

        public:
            Holder(SpinLock * s) : m_pSpinLock(s)
            {
                m_pSpinLock->GetLock();
            }

            ~Holder()
            {
                m_pSpinLock->FreeLock();
            }
        };
    };

    typedef SpinLock::Holder SpinLockHolder;
}
