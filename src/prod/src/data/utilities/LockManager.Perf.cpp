// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'lmPT'

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class LockManagerPerfTest
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        LockManagerPerfTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~LockManagerPerfTest()
        {
            try
            {
                ktlSystem_->Shutdown();
            }
            catch (Exception const & e)
            {
                cout << e.GetStatus() << endl;
            }
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

        ktl::Awaitable<void> AcquireReleaseExclusiveLocksAsync(LockManager & manager, ULONG32 offset, ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            auto timeout = Common::TimeSpan::FromSeconds(10);
            LockManager::SPtr managerSPtr = &manager;
            KSharedArray<ktl::Awaitable<LockControlBlock::SPtr>>::SPtr writers = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>>();

            for (ULONG32 i = 0; i < count; i++)
            {
                writers->Append(managerSPtr->AcquireLockAsync(offset, i + offset, LockMode::Enum::Exclusive, timeout));
            }

            for (ULONG32 i = 0; i < count; i++)
            {
                auto writer = co_await (*writers)[i];
                managerSPtr->ReleaseLock(*writer);
                writer->Close();
            }
        }

        ktl::Awaitable<void> AcquireReleaseSingleLockAsync(LockManager & manager, ULONG32 key, ULONG32 offset, ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            auto timeout = Common::TimeSpan::FromSeconds(10);
            LockManager::SPtr managerSPtr = &manager;

            for (ULONG32 i = 0; i < count; i++)
            {
                auto reader = co_await managerSPtr->AcquireLockAsync(static_cast<LONG64>(i + offset), key, LockMode::Enum::Shared, timeout);
                managerSPtr->ReleaseLock(*reader);
                reader->Close();
            }
        }

        void LockManagerSingleLockPerfTest(ULONG32 totalAcquires, ULONG32 numTasks)
        {
            auto timeout = Common::TimeSpan::FromSeconds(4);

            ULONG32 numLocksPerTask = totalAcquires / numTasks;

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SingleLock Tasks: {0} Locks per Task: {1}; Total Locks: {2}",
                numTasks,
                numLocksPerTask,
                totalAcquires);

            LockManager::SPtr lockManagerSPtr = nullptr;
            NTSTATUS status = LockManager::Create(GetAllocator(), lockManagerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            SyncAwait(lockManagerSPtr->OpenAsync());

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            Common::Stopwatch stopwatch;

            stopwatch.Start();

            for (ULONG32 n = 0; n < numTasks; n++)
            {
                tasks->Append(AcquireReleaseSingleLockAsync(*lockManagerSPtr, 8, n * numLocksPerTask, numLocksPerTask));
            }

            SyncAwait(TaskUtilities<void>::WhenAll(*tasks));
            stopwatch.Stop();

            SyncAwait(lockManagerSPtr->CloseAsync());

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SingleKeyPerf : {0} ms",
                stopwatch.ElapsedMilliseconds);
        }

        ktl::Awaitable<void> BatchAcquireReleaseSingleLockAsync(LockManager & manager, ULONG32 key, ULONG32 offset, ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            auto timeout = Common::TimeSpan::FromSeconds(10);
            LockManager::SPtr managerSPtr = &manager;

            KSharedArray<ktl::Awaitable<LockControlBlock::SPtr>>::SPtr readers = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>>();

            for (ULONG32 i = 0; i < count; i++)
            {
                auto reader = managerSPtr->AcquireLockAsync(static_cast<LONG64>(i + offset), key, LockMode::Enum::Shared, timeout);
                readers->Append(Ktl::Move(reader));
            }

            for (ULONG32 i = 0; i < readers->Count(); i++)
            {
                auto & readerTask = (*readers)[i];
                auto reader = co_await readerTask;
                reader->Close();
            }
        }

        void LockManagerSingleLockBatchPerfTest(ULONG32 totalAcquires, ULONG32 numTasks)
        {
            auto timeout = Common::TimeSpan::FromSeconds(4);

            ULONG32 numLocksPerTask = totalAcquires / numTasks;

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SingleLock Tasks: {0} Locks per Task: {1}; Total Locks: {2}",
                numTasks,
                numLocksPerTask,
                totalAcquires);

            LockManager::SPtr lockManagerSPtr = nullptr;
            NTSTATUS status = LockManager::Create(GetAllocator(), lockManagerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            SyncAwait(lockManagerSPtr->OpenAsync());

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            Common::Stopwatch stopwatch;

            stopwatch.Start();

            for (ULONG32 n = 0; n < numTasks; n++)
            {
                tasks->Append(BatchAcquireReleaseSingleLockAsync(*lockManagerSPtr, 8, n * numLocksPerTask, numLocksPerTask));
            }

            SyncAwait(TaskUtilities<void>::WhenAll(*tasks));
            stopwatch.Stop();

            SyncAwait(lockManagerSPtr->CloseAsync());
            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SingleKeyPerf : {0} ms",
                stopwatch.ElapsedMilliseconds);
        }

        void LockMangerWritersPerfTest(ULONG32 numOwners, ULONG32 numLocksPerOwner)
        {
            auto timeout = Common::TimeSpan::FromSeconds(4);
            ULONG32 totalLocks = numOwners * numLocksPerOwner;

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SequentialWriters Owners: {0}; Locks per Owner: {1}; Total Locks: {2}",
                numOwners,
                numLocksPerOwner,
                totalLocks);

            LockManager::SPtr lockManagerSPtr = nullptr;
            NTSTATUS status = LockManager::Create(GetAllocator(), lockManagerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            SyncAwait(lockManagerSPtr->OpenAsync());

            KSharedArray<ktl::Awaitable<LockControlBlock::SPtr>>::SPtr locksAwaitableArray = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>>();

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            ULONG64 resourceNameHash = 0;
            for (ULONG32 n = 0; n < numOwners; n++)
            {
                for (ULONG32 l = 0; l < numLocksPerOwner; l++)
                {
                    auto writer = lockManagerSPtr->AcquireLockAsync(n, resourceNameHash, LockMode::Enum::Exclusive, timeout);
                    locksAwaitableArray->Append(Ktl::Move(writer));
                    resourceNameHash++;
                }
            }

            SyncAwait(TaskUtilities<LockControlBlock::SPtr>::WhenAll(*locksAwaitableArray));
            stopwatch.Stop();

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SequentialWriters Acquired locks: {0} ms",
                stopwatch.ElapsedMilliseconds);

            stopwatch.Start();
            for (ULONG32 i = 0; i < locksAwaitableArray->Count(); i++)
            {
                auto writer = SyncAwait((*locksAwaitableArray)[i]);
                CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
                status = lockManagerSPtr->ReleaseLock(*writer);
                CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);
                writer->Close();
            }
            stopwatch.Stop();

            Trace.WriteInfo(
                BoostTestTrace,
                "LockManager_SequentialWriters Released locks: {0} ms",
                stopwatch.ElapsedMilliseconds);

            SyncAwait(lockManagerSPtr->CloseAsync());
        }

        ktl::Awaitable<void> LockManager_AcquireRelease_UntilCancelled(
            __in ULONG32 taskId,
            __in LockManager & lockManager,
            __in LONG64 & globalOpCount, 
            __in CancellationToken token)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());
            auto localReadCount = 0;
            while (!token.IsCancellationRequested)
            {
                auto reader = co_await lockManager.AcquireLockAsync(
                    taskId,
                    5,
                    LockMode::Enum::Shared,
                    Common::TimeSpan::FromSeconds(4)
                );

                lockManager.ReleaseLock(*reader);
                reader->Close();

                localReadCount++;
            }

            InterlockedAdd64(&globalOpCount, localReadCount);
        }

        ktl::Awaitable<void> LockManager_SingleKeyRead_Throughput(__in Common::TimeSpan duration, __in ULONG numTasks)
        {
            LockManager::SPtr lockManagerSPtr = nullptr;
            LockManager::Create(GetAllocator(), lockManagerSPtr);
            co_await lockManagerSPtr->OpenAsync();

            ktl::CancellationTokenSource::SPtr tokenSource = nullptr;
            ktl::CancellationTokenSource::Create(this->GetAllocator(), ALLOC_TAG, tokenSource);

            LONG64 globalOpCount = 0;

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(LockManager_AcquireRelease_UntilCancelled(i, *lockManagerSPtr, globalOpCount, tokenSource->Token));
            }

            cout << "Sleeping for " << duration.TotalSeconds() << " seconds" << endl;

            // Offloading cancellation to Common Threadpool because current KTL 
            // threadpool may starve this coroutine from running
            // TODO: Revisit with new KTL threadpool
            Common::Threadpool::Post([&] {
                tokenSource->Cancel();
            }, duration);

            co_await TaskUtilities<void>::WhenAll(*tasks);

            co_await lockManagerSPtr->CloseAsync();
            Trace.WriteInfo(
                "Perf",
                "Test Throughput: {0} ops / {1} second = {2} ops/sec",
                globalOpCount,
                duration.TotalSeconds(),
                globalOpCount / duration.TotalSeconds());
        }

    private:
        KtlSystem* ktlSystem_;
        LockManager::SPtr lockManagerSPtr_;
    };

    BOOST_FIXTURE_TEST_SUITE(LockManagerPerfTestSuite, LockManagerPerfTest)

    BOOST_AUTO_TEST_CASE(LockManagerPerf_1KOwners_1KLocks)
    {
        ULONG32 numOwners = 1000;
        ULONG32 numLocksPerOwner = 1;
        LockMangerWritersPerfTest(numOwners, numLocksPerOwner);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_200Owners_1KLocks)
    {
        ULONG32 numOwners = 200;
        ULONG32 numLocksPerOwner = 5;
        LockMangerWritersPerfTest(numOwners, numLocksPerOwner);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_1MOwners_1MLocks, *boost::unit_test::label("perf-cit"))
    {
        ULONG32 numOwners = 1000000;
        ULONG32 numLocksPerOwner = 1;
        LockMangerWritersPerfTest(numOwners, numLocksPerOwner);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_200Owners_1MLocks, *boost::unit_test::label("perf-cit"))
    {
        ULONG32 numOwners = 200;
        ULONG32 numLocksPerOwner = 5000;
        LockMangerWritersPerfTest(numOwners, numLocksPerOwner);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_1M_Concurrent, *boost::unit_test::label("perf-cit"))
    {
        const ULONG32 totalLocks = 1000000;
        const ULONG32 numTasks = 200;
        const ULONG32 count = totalLocks / numTasks;

        LockManager::SPtr lockManagerSPtr = nullptr;
        NTSTATUS status = LockManager::Create(GetAllocator(), lockManagerSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        SyncAwait(lockManagerSPtr->OpenAsync());

        KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
        ULONG32 offset = 0;

        Common::Stopwatch stopwatch;
        stopwatch.Start();

        for (ULONG32 i = 0; i < numTasks; i++)
        {
            tasks->Append(AcquireReleaseExclusiveLocksAsync(*lockManagerSPtr, offset, count));
            offset += count;
        }

        SyncAwait(TaskUtilities<void>::WhenAll(*tasks));

        SyncAwait(lockManagerSPtr->CloseAsync());
        Trace.WriteInfo(
            BoostTestTrace,
            "Concurrent AcquireRelease: {0} ms",
            stopwatch.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_SingleKey_1M_200Tasks, *boost::unit_test::label("perf-cit"))
    {
        // TODO: Configure test to actually run 200 tasks concurrently. Currently ~12 run together
        LockManagerSingleLockPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(LockManagerPerf_SingleKey_Throughput)
    {
        SyncAwait(LockManager_SingleKeyRead_Throughput(Common::TimeSpan::FromSeconds(180), 12));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
