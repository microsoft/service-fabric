// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'tsTP'

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    struct ReaderWriterAsyncLockTest
    {
        ReaderWriterAsyncLockTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~ReaderWriterAsyncLockTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    enum LockType { Reader, Writer };

    // Verifies the ReaderWriterAsyncLock state (number of active/waiting readers and writers).
    static void VerifyLockState(
        __in ReaderWriterAsyncLock& rwLock, 
        __in int activeReaders, 
        __in int waitingReaders, 
        __in bool activeWriter, 
        __in int waitingWriters)
    {
        CODING_ERROR_ASSERT(activeReaders == rwLock.ActiveReaderCount);
        CODING_ERROR_ASSERT(waitingReaders == rwLock.WaitingReaderCount);
        CODING_ERROR_ASSERT(activeWriter == rwLock.IsActiveWriter);
        ASSERT_IFNOT(waitingWriters == rwLock.WaitingWriterCount, "{0}=={1}", waitingWriters, rwLock.WaitingWriterCount);
    }

    static Awaitable<void> WriterAsync(
        __in ReaderWriterAsyncLock& readerWriterLock, 
        __in int timeout, 
        __in KAllocator & allocator)
    {
        bool result = co_await readerWriterLock.AcquireWriteLockAsync(timeout);
        CODING_ERROR_ASSERT(result);
        // Simulate work
        KTimer::SPtr localTimer = nullptr;
        NTSTATUS status = KTimer::Create(localTimer, allocator, KTL_TAG_TEST);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_await localTimer->StartTimerAsync(100, nullptr);
        readerWriterLock.ReleaseWriteLock();
    }

    static Awaitable<void> ReaderAsync(
        __in ReaderWriterAsyncLock& readerWriterLock, 
        __in int timeout, 
        __in KAllocator & allocator)
    {
        bool result = co_await readerWriterLock.AcquireReadLockAsync(timeout);
        CODING_ERROR_ASSERT(result);
        // Simulate work
        KTimer::SPtr localTimer = nullptr;
        NTSTATUS status = KTimer::Create(localTimer, allocator, KTL_TAG_TEST);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_await localTimer->StartTimerAsync(100, nullptr);
        readerWriterLock.ReleaseReadLock();
    }

    static Awaitable<void> VerifyLockIsFunctionalAsync(__in ReaderWriterAsyncLock& rwLock)
    {
        bool verifyReadLockAcquired = co_await rwLock.AcquireReadLockAsync(50);
        CODING_ERROR_ASSERT(verifyReadLockAcquired);
        rwLock.ReleaseReadLock();

        bool verifyWriteLockAcquired = co_await rwLock.AcquireWriteLockAsync(50);
        CODING_ERROR_ASSERT(verifyWriteLockAcquired);
        rwLock.ReleaseWriteLock();
    }

    static Awaitable<void> LockWorkloadAsync(
        __in ReaderWriterAsyncLock & readerWriterLock,
        __in bool writer,
        __in bool canTimeout,
        __in int iterationCount,
        __in AwaitableCompletionSource<void> & startSignal,
        __in KAllocator & allocator,
        __in int lockTime = 0)
    {
        ReaderWriterAsyncLock::SPtr rwLockSPtr = &readerWriterLock;
        AwaitableCompletionSource<void>::SPtr startSignalSPtr = &startSignal;

        co_await startSignalSPtr->GetAwaitable();

        int timeout = canTimeout ? 50 : 999999;

        int successCount = 0;
        for (int i = 0; i < iterationCount; i++)
        {
            if (i != 0)
            {
                co_await KTimer::StartTimerAsync(allocator, ALLOC_TAG, 2, nullptr);
            }

            bool isLockTaken = co_await (writer ? rwLockSPtr->AcquireWriteLockAsync(timeout) : rwLockSPtr->AcquireReadLockAsync(timeout));
            CODING_ERROR_ASSERT(canTimeout || isLockTaken);

            if (!isLockTaken)
            {
                continue;
            }

            if (lockTime > 0)
            {
                co_await KTimer::StartTimerAsync(allocator, ALLOC_TAG, lockTime, nullptr);
            }

            successCount++;
            
            writer ? rwLockSPtr->ReleaseWriteLock() : rwLockSPtr->ReleaseReadLock();
        }

        cout << "IsWriter: " << writer << ", CanTimeout: " << canTimeout << ", LockTime: " << lockTime << ", Count: " << successCount << endl;
    }

    static void ShortSleep()
    {
        KNt::Sleep(300);
    }

    static void LongSleep()
    {
        KNt::Sleep(10'000);
    }

    static ktl::Awaitable<void> AcquireLock_HandOffToTimedOutWriter_LockMustNotBeAbandonedAsync(
        __in KAllocator & allocator,
        __in bool isFirstWriter, 
        __in bool isSecondWriter, 
        __in int numberOfTimedOutLocks = 1)
    {
        ReaderWriterAsyncLock::SPtr rwLock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(ShortSleep, allocator, ALLOC_TAG, rwLock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool isLockOneAcquired = co_await (isFirstWriter ? rwLock->AcquireWriteLockAsync(50) : rwLock->AcquireReadLockAsync(50));
        CODING_ERROR_ASSERT(isLockOneAcquired);

        KArray<ktl::Awaitable<bool>> timedOutLockTasks(allocator, numberOfTimedOutLocks);

        for (int i = 0; i < numberOfTimedOutLocks; i++)
        {
            timedOutLockTasks.Append(isSecondWriter ? rwLock->AcquireWriteLockAsync(50) : rwLock->AcquireReadLockAsync(50));
        }

        if (isFirstWriter)
        {
            rwLock->ReleaseWriteLock();
        }
        else
        {
            rwLock->ReleaseReadLock();
        }

        for (int i = 0; i < numberOfTimedOutLocks; i++)
        {
            bool acquired = co_await timedOutLockTasks[i];
            CODING_ERROR_ASSERT(acquired);

            if (isSecondWriter)
            {
                rwLock->ReleaseWriteLock();
            }
            else
            {
                rwLock->ReleaseReadLock();
            }
        }
        
        co_await VerifyLockIsFunctionalAsync(*rwLock);

        rwLock->Close();
    }

    BOOST_FIXTURE_TEST_SUITE(ReaderWriterLockTestSuite, ReaderWriterAsyncLockTest)

    BOOST_AUTO_TEST_CASE(AquireLock_AquireWriteThenRead_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        SyncAwait(lock->AcquireWriteLockAsync(0));
        lock->ReleaseWriteLock();

        SyncAwait(lock->AcquireReadLockAsync(0));
        lock->ReleaseReadLock();

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AquireReadLock_WithOneReader_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr    lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool result = SyncAwait(lock->AcquireReadLockAsync(100));
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 1, 0, false, 0);
        lock->ReleaseReadLock();
        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AquireWriteLock_WithOneWriter_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr    lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool result = SyncAwait(lock->AcquireWriteLockAsync(100));
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 0, 0, true, 0);
        lock->ReleaseWriteLock();
        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireReadLock_AfterLockClosed_Throws)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr    lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool hasEx = false;
        lock->Close();
        try
        {
            SyncAwait(lock->AcquireReadLockAsync(99999));//TODO: need infinite time out.
        }
        catch (Exception & e) {
            hasEx = true;
            CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
        }

        CODING_ERROR_ASSERT(hasEx);
    }
    
    BOOST_AUTO_TEST_CASE(AcquireWriteLock_AfterLockClosed_Throws)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr    lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        bool hasEx = false;
        lock->Close();
        
        try {
            SyncAwait(lock->AcquireWriteLockAsync(99999));//TODO: need infinite time out.
        }
        catch (Exception & e) {
            hasEx = true;
            CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
        }

        CODING_ERROR_ASSERT(hasEx);
    }
    
    BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingWriter_AfterLockClosed_Throws)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool result = true;
        result = SyncAwait(lock->AcquireWriteLockAsync(0));
        CODING_ERROR_ASSERT(result);
        Awaitable<bool> requestLockDuringClose = lock->AcquireWriteLockAsync(9999);
        lock->Close();
        try 
        {
            SyncAwait(requestLockDuringClose);
        }
        catch (Exception & e) {
            CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
        }
    }

    BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingReader_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Awaitable<bool> reader1 = lock->AcquireReadLockAsync(0);
        bool result = SyncAwait(reader1);
        CODING_ERROR_ASSERT(result);

        Awaitable<bool> reader2 = lock->AcquireReadLockAsync(0);
        result = SyncAwait(reader2);
        CODING_ERROR_ASSERT(result);

        lock->ReleaseReadLock();

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireWriteLock_AfterExistingWriterReleases_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Awaitable<bool> writer1 = lock->AcquireWriteLockAsync(100);
        bool result = SyncAwait(writer1);
        CODING_ERROR_ASSERT(result);

        Awaitable<bool> writer2 = lock->AcquireWriteLockAsync(100);
        VerifyLockState(*lock, 0, 0, true, 1);

        lock->ReleaseWriteLock();
        result = SyncAwait(writer2);
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 0, 0, true, 0);

        lock->ReleaseWriteLock();
        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireReadLock_AllowsMutipleReaders_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (int i = 0; i < 100; i++)
        {
            bool result = SyncAwait(lock->AcquireReadLockAsync(100));
            CODING_ERROR_ASSERT(result);
            VerifyLockState(*lock, i + 1, 0, false, 0);
        }

        VerifyLockState(*lock, 100, 0, false, 0);

        for (int i = 100; i > 0; i--)
        {
            lock->ReleaseReadLock();
            VerifyLockState(*lock, i - 1, 0, false, 0);
        }

        VerifyLockState(*lock, 0, 0, false, 0);
        
        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireWriteLock_AllowsSequentialWriters_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        int count = 100;
        vector<Awaitable<bool>> writers;

        for (int i = 0; i < count; i++)
        {
            writers.push_back(lock->AcquireWriteLockAsync(10'000));
            VerifyLockState(*lock, 0, 0, true, i);
        }

        VerifyLockState(*lock, 0, 0, true, count - 1);

        bool result = true;
        for (int i = 0; i < writers.size(); i++)
        {
            result = SyncAwait(writers[i]);
            CODING_ERROR_ASSERT(result);
            VerifyLockState(*lock, 0, 0, true, static_cast<int>(writers.size()) - i - 1);
            lock->ReleaseWriteLock();
        }

        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(MutipleReaderWriter_AquireLockAndWait_WaitingWritersPreferredOverWaitingReaders)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool result = true;
        vector<Awaitable<bool>> writers;
        // Acquire read lock
        result = SyncAwait(lock->AcquireReadLockAsync(100));
        CODING_ERROR_ASSERT(result);

        //// Request write and read locks
        Awaitable<bool> writer1 = lock->AcquireWriteLockAsync(100);
        Awaitable<bool> reader1 = lock->AcquireReadLockAsync(100);
        Awaitable<bool> reader2 = lock->AcquireReadLockAsync(100);
        Awaitable<bool> writer2 = lock->AcquireWriteLockAsync(100);

        //// The reader should be active, with 2 waiting readers and writers
        VerifyLockState(*lock, 1, 2, false, 2);

        //// Release read lock
        lock->ReleaseReadLock();

        //// The writers should be preferred over the readers, with writers allowed in order of request
        result = SyncAwait(writer1);
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 0, 2, true, 1);

        //// Release first write lock
        lock->ReleaseWriteLock();

        //// Verify the next write lock is allowed through
        result = SyncAwait(writer2);
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 0, 2, true, 0);

        //// Release second write lock
        lock->ReleaseWriteLock();

        //// Verify both readers are allowed through
        result = SyncAwait(reader1);
        CODING_ERROR_ASSERT(result);
        result = SyncAwait(reader2);
        CODING_ERROR_ASSERT(result);
        VerifyLockState(*lock, 2, 0, false, 0);

        //// Release the read locks
        lock->ReleaseReadLock();
        lock->ReleaseReadLock();
        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(MutipleReaderWriter_AfterLockClosed_Throws)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        int count = 100;
        vector<Awaitable<void>> taskList;

        for (int i = 0; i < count; i++)
        {
            taskList.push_back(WriterAsync(*lock, -1, allocator));
            taskList.push_back(ReaderAsync(*lock, -1, allocator));
        }

        int exCount = 0;
        KTimer::SPtr localTimer;
        NTSTATUS timerStatus = KTimer::Create(localTimer, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(timerStatus))
        {
            CODING_ERROR_ASSERT(false);
        }

        SyncAwait(localTimer->StartTimerAsync(1000, nullptr));

        lock->Close();
        for (int i = 0; i < taskList.size(); i++)
        {
            try {
                SyncAwait(taskList[i]);
            }
            catch (Exception & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
                exCount++;
            }
        }
    }

    BOOST_AUTO_TEST_CASE(MutipleReaderWriter_AfterAllRelease_VerifyLocksStateTrackedCorrectly)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Acquire write lock
        bool result = true;
        result = SyncAwait(lock->AcquireWriteLockAsync(999999));
        CODING_ERROR_ASSERT(result);
        vector<Awaitable<bool>> taskList;

        // Request read locks
        for (int i = 0; i < 10; i++)
        {
            taskList.push_back(lock->AcquireReadLockAsync(999999));
        }

        // Request write locks
        for (int i = 0; i < 10; i++)
        {
            taskList.push_back(lock->AcquireWriteLockAsync(999999));
        }

        VerifyLockState(*lock, 0, 10, true, 10);

        // Complete one writer
        lock->ReleaseWriteLock();

        VerifyLockState(*lock, 0, 10, true, 9);

        // Complete all the writers
        for (int i = 10; i < 20; i++)
        {
            SyncAwait(taskList[i]);
            lock->ReleaseWriteLock();
        }

        VerifyLockState(*lock, 10, 0, false, 0);

        // Complete all the readers
        for (int i = 0; i < 10; i++)
        {
            SyncAwait(taskList[i]);
            lock->ReleaseReadLock();
        }

        VerifyLockState(*lock, 0, 0, false, 0);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(MutipleReaderWriter_ConcurrentAquireLockWithNoTimeout_Succeeds)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        vector<Awaitable<void>> taskList;
        int count = 100;

        for (int i = 0; i < count; i++)
        {
            taskList.push_back(WriterAsync(*lock, 999999, allocator));
            taskList.push_back(ReaderAsync(*lock, 999999, allocator));
        }

        for (int i = 0; i < count; i++)
        {
            taskList.push_back(ReaderAsync(*lock, 999999, allocator));
            taskList.push_back(WriterAsync(*lock, 999999, allocator));
        }

        for (int i = 0; i < taskList.size(); i++)
        {
            SyncAwait(taskList[i]);
        }

        SyncAwait(VerifyLockIsFunctionalAsync(*lock));

        lock->Close();
    }
    
    BOOST_AUTO_TEST_CASE(AcquireWriteLock_WhileWriteHeld_ShouldTimeout)
    {
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(GetAllocator(), KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // This acquire should succeed
        bool acquired = SyncAwait(lock->AcquireWriteLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired);

        // This acquire should timeout
        acquired = SyncAwait(lock->AcquireWriteLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired == false);

        lock->ReleaseWriteLock();

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireWriteLock_WhileReadHeld_ShouldTimeout)
    {
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(GetAllocator(), KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // This acquire should succeed
        bool acquired = SyncAwait(lock->AcquireReadLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired);

        // This acquire should timeout
        acquired = SyncAwait(lock->AcquireWriteLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired == false);

        lock->ReleaseReadLock();

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireReadLock_WhileWriteHeld_ShouldTimeout)
    {
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(GetAllocator(), KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // This acquire should succeed
        bool acquired = SyncAwait(lock->AcquireWriteLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired);

        // This acquire should timeout
        acquired = SyncAwait(lock->AcquireReadLockAsync(1'000));
        CODING_ERROR_ASSERT(acquired == false);

        lock->ReleaseWriteLock();

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(LocksRespectTimeout_ShouldSucceed)
    {
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(GetAllocator(), KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG32 i = 0; i < 10; i++)
        {
            // Acquire write lock, verify that a writer times out, then release the write lock and allow a writer through
            {
                bool result = SyncAwait(lock->AcquireWriteLockAsync(100));
                CODING_ERROR_ASSERT(result == true);

                result = SyncAwait(lock->AcquireWriteLockAsync(100));
                CODING_ERROR_ASSERT(result == false);

                lock->ReleaseWriteLock();

                result = SyncAwait(lock->AcquireWriteLockAsync(100));
                CODING_ERROR_ASSERT(result == true);

                lock->ReleaseWriteLock();
            }

            // Acquire write lock, verify that a reader times out, then release the write lock and allow a reader through
            {
                bool result = SyncAwait(lock->AcquireWriteLockAsync(100));
                CODING_ERROR_ASSERT(result == true);

                result = SyncAwait(lock->AcquireReadLockAsync(100));
                CODING_ERROR_ASSERT(result == false);

                lock->ReleaseWriteLock();

                result = SyncAwait(lock->AcquireReadLockAsync(100));
                CODING_ERROR_ASSERT(result == true);

                lock->ReleaseReadLock();
            }
        }

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(MultipleReaderWriter_ConcurrentLockAquires_WithTimeout_ShouldSucceed)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        vector<Awaitable<void>> taskList;
        int count = 100;

        for (int i = 0; i < count; i++)
        {
            taskList.push_back(WriterAsync(*lock, 25'000, allocator));
            taskList.push_back(ReaderAsync(*lock, 25'000, allocator));
        }

        for (int i = 0; i < count; i++)
        {
            taskList.push_back(ReaderAsync(*lock, 25'000, allocator));
            taskList.push_back(WriterAsync(*lock, 25'000, allocator));
        }

        for (int i = 0; i < taskList.size(); i++)
        {
            SyncAwait(taskList[i]);
        }

        SyncAwait(VerifyLockIsFunctionalAsync(*lock));

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(AcquireLocksAfterTimeout_ShouldSucceed)
    {
        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();
        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool result = SyncAwait(lock->AcquireWriteLockAsync(0));
        CODING_ERROR_ASSERT(result);

        for (int i = 0; i < 10; i++)
        {
            result = SyncAwait(lock->AcquireWriteLockAsync(0));
            CODING_ERROR_ASSERT(result == false);

            result = SyncAwait(lock->AcquireReadLockAsync(0));
            CODING_ERROR_ASSERT(result == false);
        }

        lock->ReleaseWriteLock();

        for (int i = 0; i < 10; i++)
        {
            result = SyncAwait(lock->AcquireReadLockAsync(0));
            CODING_ERROR_ASSERT(result);
        }

        for (int i = 0; i < 10; i++)
        {
            lock->ReleaseReadLock();
        }

        result = SyncAwait(lock->AcquireWriteLockAsync(0));
        CODING_ERROR_ASSERT(result);

        lock->Close();
    }

    BOOST_AUTO_TEST_CASE(RandomWorkload_LockMustNotBeAbandoned)
    {
        int fastLockCount = 1000;
        int slowLockCount = 200;

        KAllocator& allocator = ReaderWriterAsyncLockTest::GetAllocator();

        ReaderWriterAsyncLock::SPtr lock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(allocator, KTL_TAG_TEST, lock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        AwaitableCompletionSource<void>::SPtr startSignal = nullptr;
        AwaitableCompletionSource<void>::Create(allocator, ALLOC_TAG, startSignal);

        KArray<Awaitable<void>> workloads(GetAllocator(), 8);
        
        workloads.Append(LockWorkloadAsync(*lock, false, false, fastLockCount, *startSignal, allocator));
        workloads.Append(LockWorkloadAsync(*lock, true, false, fastLockCount, *startSignal, allocator));
        workloads.Append(LockWorkloadAsync(*lock, false, true, fastLockCount, *startSignal, allocator));
        workloads.Append(LockWorkloadAsync(*lock, true, true, fastLockCount, *startSignal, allocator));
        workloads.Append(LockWorkloadAsync(*lock, false, false, slowLockCount, *startSignal, allocator, 100));
        workloads.Append(LockWorkloadAsync(*lock, true, false, slowLockCount, *startSignal, allocator, 100));
        workloads.Append(LockWorkloadAsync(*lock, false, true, slowLockCount, *startSignal, allocator, 100));
        workloads.Append(LockWorkloadAsync(*lock, true, true, slowLockCount, *startSignal, allocator, 100));

        startSignal->Set();

        for (ULONG32 i = 0; i < workloads.Count(); i++)
        {
            SyncAwait(workloads[i]);
        }

        SyncAwait(VerifyLockIsFunctionalAsync(*lock));

        lock->Close();
    }
    

    // Following tests are sensitive to thread scheduling order which appverifier changes
    // The tests expect the first lock to signal before the second locks timeout
    BOOST_AUTO_TEST_CASE(Handoff_WriterToTimedOutWriter_LockMustNotBeAbandoned, *boost::unit_test::label("no_appverif"))
    {
        int iterations = 64;

        for (int i = 0; i < iterations; i++)
        {
            SyncAwait(AcquireLock_HandOffToTimedOutWriter_LockMustNotBeAbandonedAsync(GetAllocator(), true, true));
        }
    }

    BOOST_AUTO_TEST_CASE(Handoff_ReaderToTimedOutWriter_LockMustNotBeAbandoned, *boost::unit_test::label("no_appverif"))
    {
        int iterations = 64;

        for (int i = 0; i < iterations; i++)
        {
            SyncAwait(AcquireLock_HandOffToTimedOutWriter_LockMustNotBeAbandonedAsync(GetAllocator(), false, true));
        }
    }

    BOOST_AUTO_TEST_CASE(Handoff_WriterToTimedOutReader_LockMustNotBeAbandoned, *boost::unit_test::label("no_appverif"))
    {
        int iterations = 64;

        for (int i = 0; i < iterations; i++)
        {
            SyncAwait(AcquireLock_HandOffToTimedOutWriter_LockMustNotBeAbandonedAsync(GetAllocator(), true, false));
        }
    }

    BOOST_AUTO_TEST_CASE(Handoff_WriterToManyTimedOutReader_LockMustNotBeAbandoned, *boost::unit_test::label("no_appverif"))
    {
        int iterations = 64;

        for (int i = 0; i < iterations; i++)
        {
            SyncAwait(AcquireLock_HandOffToTimedOutWriter_LockMustNotBeAbandonedAsync(GetAllocator(), true, false, 64));
            cout << endl << endl;
        }

    }

    BOOST_AUTO_TEST_CASE(Close_ReadLockGranted_WaitingWriteLock_WaitingRequestMustThrow)
    {
        ReaderWriterAsyncLock::SPtr rwLock = nullptr;
        NTSTATUS status = ReaderWriterAsyncLock::Create(LongSleep, GetAllocator(), ALLOC_TAG, rwLock);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        SyncAwait(rwLock->AcquireReadLockAsync(100));
        auto timeoutWriter = rwLock->AcquireWriteLockAsync(100);

        rwLock->Close();
        rwLock->ReleaseReadLock();

        try
        {
            SyncAwait(timeoutWriter);
        }
        catch (Exception const & e)
        {
            CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
        }
    }
        
    BOOST_AUTO_TEST_SUITE_END()
}

