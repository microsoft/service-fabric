#include <unistd.h>
#include "MinPal.h"
#include "Volatile.h"
#include "Synch.h"
#include "Interlock.h"
#include "UnfairSemaphore.h"
#include "Threadpool.h"

namespace KtlThreadpool {

    UnfairSemaphore::UnfairSemaphore(int numProcessors, int maxCount)
    {
        _ASSERTE(maxCount <= 0x7fff); //counts need to fit in signed 16-bit ints

        m_counts.asLongLong = 0;
        m_maxCount = maxCount;
        m_numProcessors = numProcessors;
        m_sem.Create(0);
    }

    bool UnfairSemaphore::UpdateCounts(Counts newCounts, Counts currentCounts)
    {
        Counts oldCounts;
        oldCounts.asLongLong = InterlockedCompareExchange64(&m_counts.asLongLong, newCounts.asLongLong, currentCounts.asLongLong);
        if (oldCounts.asLongLong == currentCounts.asLongLong)
        {
            // we succesfully updated the counts.  Now validate what we put in.
            // Note: we can't validate these unless the CompareExchange succeeds, because
            // on x86 a VolatileLoad of m_counts is not atomic; we could end up getting inconsistent
            // values.  It's not until we've successfully stored the new values that we know for sure that
            // the old values were correct (because if they were not, the CompareExchange would have failed.
            _ASSERTE(newCounts.spinners >= 0);
            _ASSERTE(newCounts.countForSpinners >= 0);
            _ASSERTE(newCounts.waiters >= 0);
            _ASSERTE(newCounts.countForWaiters >= 0);
            _ASSERTE(newCounts.countForSpinners + newCounts.countForWaiters <= m_maxCount);

            return true;
        }
        else
        {
            // we lost a race with some other thread, and will need to try again.
            return false;
        }
    }

    void UnfairSemaphore::Release(int countToRelease)
    {
        while (true)
        {
            Counts currentCounts, newCounts;
            currentCounts.asLongLong = VolatileLoad(&m_counts.asLongLong);
            newCounts = currentCounts;

            int remainingCount = countToRelease;

            // First, prefer to release existing spinners,
            // because a) they're hot, and b) we don't need a kernel transition to release them.
            int spinnersToRelease = __max(0, __min(remainingCount, currentCounts.spinners - currentCounts.countForSpinners));
            newCounts.countForSpinners += spinnersToRelease;
            remainingCount -= spinnersToRelease;

            // Next, prefer to release existing waiters
            int waitersToRelease = __max(0, __min(remainingCount, currentCounts.waiters - currentCounts.countForWaiters));
            newCounts.countForWaiters += waitersToRelease;
            remainingCount -= waitersToRelease;

            // Finally, release any future spinners that might come our way
            newCounts.countForSpinners += remainingCount;

            // Try to commit the transaction
            if (UpdateCounts(newCounts, currentCounts))
            {
                // Now we need to release the waiters we promised to release
                if (waitersToRelease > 0)
                {
                    BOOL success = m_sem.Release(waitersToRelease);
                    _ASSERTE(success);
                }
                break;
            }
        }
    }

    bool UnfairSemaphore::Wait(DWORD timeout)
    {
        while (true)
        {
            Counts currentCounts, newCounts;
            currentCounts.asLongLong = VolatileLoad(&m_counts.asLongLong);
            newCounts = currentCounts;

            // First, just try to grab some count.
            if (currentCounts.countForSpinners > 0)
            {
                newCounts.countForSpinners--;
                if (UpdateCounts(newCounts, currentCounts))
                    return true;
            }
            else
            {
                // No count available, become a spinner
                newCounts.spinners++;
                if (UpdateCounts(newCounts, currentCounts))
                    break;
            }
        }

        // Now we're a spinner.
        nice(19);
        int spincount=0;
        while (true)
        {
            Counts currentCounts, newCounts;

            currentCounts.asLongLong = VolatileLoad(&m_counts.asLongLong);
            newCounts = currentCounts;

            if (currentCounts.countForSpinners > 0)
            {
                newCounts.countForSpinners--;
                newCounts.spinners--;
                if (UpdateCounts(newCounts, currentCounts))
                {
                    return true;
                }
            }
            else
            {
                // exponential nanosleep: 1  2  4  8  16 16 16 16 16 (ms)
                if (spincount >= 8 || currentCounts.spinners >= m_numProcessors/2)
                {
                    newCounts.spinners--;
                    newCounts.waiters++;
                    if (UpdateCounts(newCounts, currentCounts))
                        break;
                }
                else
                {
                    struct timespec tim, tim2;
                    tim.tv_sec = 0;
                    tim.tv_nsec = (1 << (spincount < 4 ? spincount : 4)) * 1000000;
                    nanosleep(&tim , &tim2);
                    spincount++;
                }   
            }
        }
        nice(-19);

        // Now we're a waiter
        bool result = m_sem.Wait(timeout);

        while (true)
        {
            Counts currentCounts, newCounts;

            currentCounts.asLongLong = VolatileLoad(&m_counts.asLongLong);
            newCounts = currentCounts;

            newCounts.waiters--;

            if (result)
                newCounts.countForWaiters--;

            if (UpdateCounts(newCounts, currentCounts))
                return result;
        }
    }
}
