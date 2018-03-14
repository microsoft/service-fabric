// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "MinPal.h"
#include "Synch.h"

namespace Threadpool {

    class UnfairSemaphore
    {
    private:
        BYTE padding1[64];

        union Counts
        {
            struct
            {
                int spinners         : 16; //how many threads are currently spin-waiting for this semaphore?
                int countForSpinners : 16; //how much of the semaphore's count is availble to spinners?
                int waiters          : 16; //how many threads are blocked in the OS waiting for this semaphore?
                int countForWaiters  : 16; //how much count is available to waiters?
            };
            LONGLONG asLongLong;
        } m_counts;

    private:
#if defined(THREADPOOL_ENABLE_LIFO_SEMAPHORE)
        LifoSemaphore m_sem;  //waiters wait on this
#else
        Semaphore m_sem;  //waiters wait on this
#endif

        // padding to ensure we get our own cache line
        BYTE padding2[64];

        int m_maxCount;

        int m_numProcessors;

        bool UpdateCounts(Counts newCounts, Counts currentCounts);

    public:

        UnfairSemaphore(int numProcessors, int maxCount);

        // no destructor - Semaphore will close itself in its own destructor.
        //~UnfairSemaphore() { }

        void Release(int countToRelease);

        bool Wait(DWORD timeout);
    };
}
