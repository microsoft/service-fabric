// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class AsyncLockTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        AsyncLockTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~AsyncLockTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

        AsyncLock::SPtr CreateAsyncLock()
        {
            AsyncLock::SPtr lock;

            NTSTATUS status = AsyncLock::Create(
                GetAllocator(),
                'TEST',
                lock);

            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "Failed to create async write lock with {0}",
                status);

            return lock;
        }

    public:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(AsyncLockTestSuite, AsyncLockTest)

    BOOST_AUTO_TEST_CASE(NewLockMustAcquireImmediately)
    {
        AsyncLock::SPtr lock = CreateAsyncLock();
    
        bool result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::MaxValue));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");
        lock->ReleaseLock();
    }

    BOOST_AUTO_TEST_CASE(SerialAcquire)
    {
        AsyncLock::SPtr lock = CreateAsyncLock();
    
        bool result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::FromMilliseconds(1)));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");
        lock->ReleaseLock();

        result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::FromMilliseconds(1)));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");
        lock->ReleaseLock();
    }

    BOOST_AUTO_TEST_CASE(LockTimeout)
    {
        AsyncLock::SPtr lock = CreateAsyncLock();
    
        bool result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::MaxValue));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");

        result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::FromMilliseconds(1)));
        VERIFY_IS_FALSE(result, L"Lock acquistion should fail");

        lock->ReleaseLock();
        result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::FromMilliseconds(1)));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");
        lock->ReleaseLock();
    }

    BOOST_AUTO_TEST_CASE(SingleEntrancyTest)
    {
        AsyncLock::SPtr lock = CreateAsyncLock();
    
        bool result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::MaxValue));
        VERIFY_IS_TRUE(result, L"Lock acquistion should pass");

        Common::atomic_long countOfAcquired(1);
        Common::atomic_long totalCount(0);
        
        Common::Random r;

        Common::Threadpool::Post([&]
        {
            long count = --countOfAcquired;
            VERIFY_IS_TRUE(count == 0, L"Count must be 0");
            lock->ReleaseLock();
        },
        Common::TimeSpan::FromMilliseconds(r.Next(10)));

        int threadCount = 200;

        // While the above is acquired, start many threads that wait on the same lock and ensure only 1 of them acquires it successfully each time
        for (int i = 0; i < threadCount; i++)
        {
            Common::Threadpool::Post([&]
            {
                bool result = SyncAwait(lock->AcquireAsync(Common::TimeSpan::MaxValue));
                VERIFY_IS_TRUE(result, L"Lock acqusition should pass");

                long count = ++countOfAcquired;
                VERIFY_IS_TRUE(count == 1, L"Count must be 1");
                Sleep(10);
                count = --countOfAcquired;
                VERIFY_IS_TRUE(count == 0, L"Count must be 0");

                lock->ReleaseLock();
                totalCount++;
            },
            Common::TimeSpan::FromMilliseconds(r.Next(10)));
        }

        while (totalCount.load() < threadCount)
        {
            Sleep(1000);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
