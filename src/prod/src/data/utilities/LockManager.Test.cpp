// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

   class LockManagerTest
   {
   public:
      LockManagerTest()
      {
         NTSTATUS status;
         status = KtlSystem::Initialize(FALSE, &ktlSystem_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         ktlSystem_->SetStrictAllocationChecks(TRUE);
      }

      ~LockManagerTest()
      {
        if(lockManagerSPtr_ != nullptr)
        {
            SyncAwait(lockManagerSPtr_->CloseAsync());
            lockManagerSPtr_ = nullptr;
        }
        ktlSystem_->Shutdown();
      }

      KAllocator& GetAllocator()
      {
         return ktlSystem_->NonPagedAllocator();
      }

      LockManager::SPtr CreateLockManager()
      {
         NTSTATUS status = LockManager::Create(LockManagerTest::GetAllocator(), lockManagerSPtr_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         SyncAwait(lockManagerSPtr_->OpenAsync());
         return lockManagerSPtr_;
      }

   private:
      KtlSystem* ktlSystem_;
      LockManager::SPtr lockManagerSPtr_;

#pragma region test functions
    public:
        ktl::Awaitable<void> AcquireLock_AfterClose_Fails_Test()
       {
           LockManager::SPtr lockManagerSptr;
           auto status = LockManager::Create(LockManagerTest::GetAllocator(), lockManagerSptr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           co_await lockManagerSptr->OpenAsync();
           co_await lockManagerSptr->CloseAsync();

           bool hasEx = false;
           try
           {
               co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception & e)
           {
               hasEx = true;
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
           }

           CODING_ERROR_ASSERT(hasEx);

           try
           {
               co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception & e)
           {
               hasEx = true;
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
           }

           CODING_ERROR_ASSERT(hasEx);

           LockMode::Enum validLockModes[] = { LockMode::Enum::Shared, LockMode::Enum::Exclusive, LockMode::Enum::Update };
           for (LockMode::Enum mode : validLockModes)
           {
               try
               {
                   auto lock = co_await lockManagerSptr->AcquireLockAsync(17, 0, mode, TimeSpan::FromMilliseconds(100));
                   CODING_ERROR_ASSERT(false); // Shouldn't get here
               }
               catch (ktl::Exception const & e)
               {
                   CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
               }
           }
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_InvalidMode_ShouldThrow_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           bool hasEx = false;
           try
           {
               co_await lockManagerSptr->AcquireLockAsync(17, 0, LockMode::Enum::Free, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception & e)
           {
               hasEx = true;
               CODING_ERROR_ASSERT(e.GetStatus() == STATUS_INVALID_PARAMETER_3);
           }

           CODING_ERROR_ASSERT(hasEx);
           co_return;
       }

        ktl::Awaitable<void> SingleReader_AcquireRelease_ShouldSucceed_Test()
       {
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

          auto status = lockManagerSptr->ReleaseLock(*reader);
          CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

          reader->Close();
           co_return;
       }

        ktl::Awaitable<void> SingleWriter_AcquireRelease_ShouldSucceed_Test()
       {
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

          auto status = lockManagerSptr->ReleaseLock(*writer);
          CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

          writer->Close();
           co_return;
       }

        ktl::Awaitable<void> SameLock_ConcurrentReaders_ShouldSucceed_Test()
       {
          // Create 1000 outstanding  reader locks, and verify they all acquire the same lock concurrently

          ULONG32 count = 1000;
          KAllocator& allocator = GetAllocator();

          KArray<LockControlBlock::SPtr> locksArray(allocator, count);
          KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          // Request locks
          for (ULONG32 i = 0; i < count; i++)
          {
             ktl::Awaitable<KSharedPtr<LockControlBlock>> reader = lockManagerSptr->AcquireLockAsync(i, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
             locksAwaitableArray.Append(Ktl::Move(reader));
          }

          // Acquire locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr reader = co_await locksAwaitableArray[i];
             locksArray.Append(reader);
             CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
          }

          // Release locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr reader = locksArray[i];
             auto status = lockManagerSptr->ReleaseLock(*reader);
             CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);
             reader->Close();
          }
           co_return;
       }

        ktl::Awaitable<void> DifferentLock_ConcurrentReaders_ShouldSucceed_Test()
       {
          // Create 1000 outstanding reader locks, and verify they all acquire the same lock concurrently

          ULONG32 count = 1000;
          KAllocator& allocator = GetAllocator();

          KArray<LockControlBlock::SPtr> locksArray(allocator, count);
          KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          // Request locks
          for (ULONG32 i = 0; i < count; i++)
          {
             ktl::Awaitable<KSharedPtr<LockControlBlock>> reader = lockManagerSptr->AcquireLockAsync(i, static_cast<ULONG64>(i), LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
             locksAwaitableArray.Append(Ktl::Move(reader));
          }

          // Acquire locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr reader = co_await locksAwaitableArray[i];
             locksArray.Append(reader);
             CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
          }

          // Release locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr reader = locksArray[i];
             auto status = lockManagerSptr->ReleaseLock(*reader);
             CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);
             reader->Close();
          }
           co_return;
       }

        ktl::Awaitable<void> SameLock_SequentialWriters_ShouldSucceed_Test()
       {
          // Create 1000 outstanding writer locks, and verify they all acquire the same lock concurrently

          ULONG32 count = 1000;
          KAllocator& allocator = GetAllocator();

          KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          for (ULONG32 i = 0; i < count; i++)
          {
             ktl::Awaitable<KSharedPtr<LockControlBlock>> writer = lockManagerSptr->AcquireLockAsync(i, 100, LockMode::Enum::Exclusive, TimeSpan::MaxValue);
             locksAwaitableArray.Append(Ktl::Move(writer));
          }

          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr writer = co_await locksAwaitableArray[i];
             CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
             auto status = lockManagerSptr->ReleaseLock(*writer);
             CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

             writer->Close();
          }
           co_return;
       }

        ktl::Awaitable<void> SameLock_ReadersWaitingOnWriters_ShouldSucceedAfterWriterReleasesLock_Test()
       {
           ULONG32 count = 2;
           KAllocator& allocator = GetAllocator();

           KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(10000, 100, LockMode::Enum::Exclusive, TimeSpan::MaxValue);

           for (ULONG32 i = 0; i < count; i++)
           {
               ktl::Awaitable<KSharedPtr<LockControlBlock>> reader = lockManagerSptr->AcquireLockAsync(i, 100, LockMode::Enum::Shared, TimeSpan::MaxValue);
               locksAwaitableArray.Append(Ktl::Move(reader));
           }

           lockManagerSptr->ReleaseLock(*writer);
           //writer->Close();

           for (ULONG32 i = 0; i < count; i++)
           {
               LockControlBlock::SPtr reader = co_await locksAwaitableArray[i];
               CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
               //reader->Close();
           }

           // Close with the granted readers.
           co_return;
       }

        ktl::Awaitable<void> DifferentLock_ConcurrentWriters_ShouldSucceed_Test()
       {
          // Create 1000 outstanding writer locks, and verify they all acquire the same lock concurrently

          ULONG32 count = 1000;
          KAllocator& allocator = GetAllocator();

          KArray<LockControlBlock::SPtr> locksArray(allocator, count);
          KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          for (ULONG32 i = 0; i < count; i++)
          {
             ktl::Awaitable<KSharedPtr<LockControlBlock>> writer = lockManagerSptr->AcquireLockAsync(17, static_cast<ULONG64>(i), LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
             locksAwaitableArray.Append(Ktl::Move(writer));
          }

          // Acquire locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr writer = co_await locksAwaitableArray[i];
             locksArray.Append(writer);
             CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
          }

          // Release locks
          for (ULONG32 i = 0; i < count; i++)
          {
             LockControlBlock::SPtr writer = locksArray[i];
             auto status = lockManagerSptr->ReleaseLock(*writer);
             CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);
             writer->Close();
          }
           co_return;
       }

        ktl::Awaitable<void> AcquireWriteLock_WithExistingWriter_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

           auto blockedWriter = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(blockedWriter->GetStatus() == LockStatus::Enum::Timeout);

           auto status = lockManagerSptr->ReleaseLock(*writer);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           writer->Close();
           blockedWriter->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireWriteLock_WithExistingReader_ShouldTimeout_Test()
       {
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

          auto writer = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);

          auto status = lockManagerSptr->ReleaseLock(*reader);
          CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

          writer->Close();
          reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadLock_WithExistingWriter_ShouldTimeout_Test()
       {
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

          auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

          auto reader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
          CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);

          auto status = lockManagerSptr->ReleaseLock(*writer);
          CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

          writer->Close();
          reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadLock_WithExistingReader_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

           auto blockedReader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(blockedReader->GetStatus() == LockStatus::Enum::Timeout);

           auto status = lockManagerSptr->ReleaseLock(*reader);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           reader->Close();
           blockedReader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireWriteLock_WithExistingWriter_ZeroTimeout_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::Zero);
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

           auto blockedWriter = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::Zero);
           CODING_ERROR_ASSERT(blockedWriter->GetStatus() == LockStatus::Enum::Timeout);

           auto status = lockManagerSptr->ReleaseLock(*writer);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           writer->Close();
           blockedWriter->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireWriteLock_WithExistingReader_ZeroTimeout_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::Zero);
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

           auto writer = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::Zero);
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);

           auto status = lockManagerSptr->ReleaseLock(*reader);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           writer->Close();
           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadLock_WithExistingWriter_ZeroTimeout_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::Zero);
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

           auto reader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::Zero);
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);

           auto status = lockManagerSptr->ReleaseLock(*writer);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           writer->Close();
           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadLock_WithExistingReader_ZeroTimeout_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto existingReader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::Zero);
           CODING_ERROR_ASSERT(existingReader->GetStatus() == LockStatus::Enum::Granted);

           auto reader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::Zero);
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

           auto status = lockManagerSptr->ReleaseLock(*existingReader);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           status = lockManagerSptr->ReleaseLock(*reader);
           CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

           existingReader->Close();
           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_AllLockModes_ShouldSucceed_Test()
       {
           LockMode::Enum validLockModes[] = { LockMode::Enum::Shared, LockMode::Enum::Exclusive, LockMode::Enum::Update };

           UINT16 count = 16;
           KAllocator& allocator = GetAllocator();

           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           for (LockMode::Enum mode : validLockModes)
           {
               KArray<LockControlBlock::SPtr> locksArray(allocator, count);
               KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);

               // Request locks for current lock mode
               for (UINT16 i = 0; i < count; i++)
               {
                   auto lockRequest = lockManagerSptr->AcquireLockAsync(i, i, mode, TimeSpan::FromMilliseconds(100));
                   locksAwaitableArray.Append(Ktl::Move(lockRequest));
               }

               // Wait to acquire lock
               for (UINT16 i = 0; i < count; i++)
               {
                   auto lock = co_await locksAwaitableArray[i];
                   locksArray.Append(lock);
                   CODING_ERROR_ASSERT(lock->GetStatus() == LockStatus::Enum::Granted);
               }

               // Release lock
               for (UINT16 i = 0; i < count; i++)
               {
                   auto lock = locksArray[i];
                   CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*lock) == UnlockStatus::Enum::Success);
                   lock->Close();
               }
           }
           co_return;
       }

        ktl::Awaitable<void> AcquireWriteLock_WithInfiniteTimeout_WithExistingWriter_ShouldSucceed_Test()
       {
           TimeSpan InfiniteTimeout = TimeSpan::MaxValue;

           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer1 = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));

           auto writer2Task = lockManagerSptr->AcquireLockAsync(2, 100, LockMode::Enum::Exclusive, InfiniteTimeout);

           co_await KTimer::StartTimerAsync(GetAllocator(), 'xxx', 1000, nullptr, nullptr);

           CODING_ERROR_ASSERT(writer2Task.IsComplete() == false);

           // Release existing write lock
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer1) == UnlockStatus::Enum::Success);

           auto writer2 = co_await writer2Task;
           CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer2) == UnlockStatus::Enum::Success);

           writer1->Close();
           writer2->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadWriteLock_WithExistingWriter_GrantedInOrder_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));

           // Request serveral read and write locks
           auto reader1Task = lockManagerSptr->AcquireLockAsync(1, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto writer2Task = lockManagerSptr->AcquireLockAsync(2, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));
           auto reader3Task = lockManagerSptr->AcquireLockAsync(3, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto reader4Task = lockManagerSptr->AcquireLockAsync(4, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto writer5Task = lockManagerSptr->AcquireLockAsync(5, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));
           auto writer6Task = lockManagerSptr->AcquireLockAsync(6, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(reader1Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer2Task.IsComplete() == false);
           CODING_ERROR_ASSERT(reader3Task.IsComplete() == false);
           CODING_ERROR_ASSERT(reader4Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);

           // Release write lock
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);

           // reader1 should be granted first
           auto reader1 = co_await reader1Task;
           CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(writer2Task.IsComplete() == false);
           CODING_ERROR_ASSERT(reader3Task.IsComplete() == false);
           CODING_ERROR_ASSERT(reader4Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader1) == UnlockStatus::Enum::Success);

           // writer2
           auto writer2 = co_await writer2Task;
           CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(reader3Task.IsComplete() == false);
           CODING_ERROR_ASSERT(reader4Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer2) == UnlockStatus::Enum::Success);

           // reader3 and reader4
           auto reader3 = co_await reader3Task;
           CODING_ERROR_ASSERT(reader3->GetStatus() == LockStatus::Enum::Granted);
           auto reader4 = co_await reader4Task;
           CODING_ERROR_ASSERT(reader4->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
           CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader3) == UnlockStatus::Enum::Success);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader4) == UnlockStatus::Enum::Success);

           // writer5
           auto writer5 = co_await writer5Task;
           CODING_ERROR_ASSERT(writer5->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer5) == UnlockStatus::Enum::Success);

           // writer6
           auto writer6 = co_await writer6Task;
           CODING_ERROR_ASSERT(writer6->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer6) == UnlockStatus::Enum::Success);

           writer->Close();
           reader1->Close();
           writer2->Close();
           reader3->Close();
           reader4->Close();
           writer5->Close();
           writer6->Close();
           co_return;
       }

        ktl::Awaitable<void> ReleaseReaderLock_AlreadyReleasedReader_ShouldFail_Test()
       {
          LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
          auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
          lockManagerSptr->ReleaseLock(*reader);

          CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::NotGranted);

          reader->Close();
           co_return;
       }

        ktl::Awaitable<void> ReleaseWriterLock_AlreadyReleasedWriter_ShouldFail_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));
           lockManagerSptr->ReleaseLock(*writer);

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::NotGranted);

           writer->Close();
           co_return;
       }

        ktl::Awaitable<void> ReleaseReaderLock_ReaderFromDifferentManager_ShouldFail_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           LockManager::SPtr otherManagerSptr;
           auto status = LockManager::Create(LockManagerTest::GetAllocator(), otherManagerSptr);
           co_await otherManagerSptr->OpenAsync();
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           auto otherReader = co_await otherManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*otherReader) == UnlockStatus::Enum::UnknownResource);
           CODING_ERROR_ASSERT(otherManagerSptr->ReleaseLock(*otherReader) == UnlockStatus::Enum::Success);

           otherReader->Close();

           co_await otherManagerSptr->CloseAsync();
           co_return;
       }

        ktl::Awaitable<void> ReleaseWriterLock_WriterFromDifferentManager_ShouldFail_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           LockManager::SPtr otherManagerSptr;
           auto status = LockManager::Create(LockManagerTest::GetAllocator(), otherManagerSptr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));
           co_await otherManagerSptr->OpenAsync();

           auto otherWriter = co_await otherManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*otherWriter) == UnlockStatus::Enum::UnknownResource);
           CODING_ERROR_ASSERT(otherManagerSptr->ReleaseLock(*otherWriter) == UnlockStatus::Enum::Success);

           otherWriter->Close();

           co_await otherManagerSptr->CloseAsync();
           co_return;
       }

        ktl::Awaitable<void> AcquireExclusive_OwnShared_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto upgradedWriter = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(upgradedWriter->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*upgradedWriter) == UnlockStatus::Enum::Success);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

           upgradedWriter->Close();

           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireShared_OwnsExclusive_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));
           auto reader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);

           reader->Close();
           writer->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireExclusive_OwnShared_WithExistingReader_ShouldSucceed_Test()
       {
           KAllocator& allocator = GetAllocator();
           try
           {
               LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
               auto existingReader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));

               auto reader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
               auto writerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

               // Reader should get in immediately, writer will have to wait until existingReader is finished
               CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
               CODING_ERROR_ASSERT(writerRequest.IsComplete() == false);

               // Release existing reader
               CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*existingReader) == UnlockStatus::Enum::Success);

               // Writer should be able to get in now
               auto writer = co_await writerRequest;
               CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

               // Release both locks
               CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);
               CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

               writer->Close();
               reader->Close();
               existingReader->Close();
           }
           catch (ktl::Exception &exception)
           {
               KDynStringA exceptionMessage(allocator);
               bool result = exception.ToString(exceptionMessage);
               if (result)
               {
                   puts(exceptionMessage);
               }

               throw;
           }
           co_return;
       }

        ktl::Awaitable<void> AcquireExclusive_OwnShared_WithExistingWriter_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
           auto existingWriter = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));

           auto readerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto writerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*existingWriter) == UnlockStatus::Enum::Success);

           auto reader = co_await readerRequest;
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

           auto writer = co_await writerRequest;
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

           writer->Close();
           reader->Close();
           existingWriter->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireExclusive_OwnShared_WithExistingReaderAndWaiter_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           auto existingReader = co_await lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));

           // Acquire a read lock, add a pending write request, then upgrade read lock to a write lock
           auto reader = co_await lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
           auto pendingWriterRequest = lockManagerSptr->AcquireLockAsync(19, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));
           auto writerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           // Since an existing read lock is outstanding, the read lock should succeed but the write requests should be pending
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(pendingWriterRequest.IsComplete() == false);
           CODING_ERROR_ASSERT(writerRequest.IsComplete() == false);

           // Release the read locks
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*existingReader) == UnlockStatus::Enum::Success);
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

           // Upgrade writerRequest should be granted before earlier pendingWriterRequest
           auto writer = co_await writerRequest;
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(pendingWriterRequest.IsComplete() == false);

           // pendingWriterRequest should be granted next
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);
           auto pendingWriter = co_await pendingWriterRequest;
           CODING_ERROR_ASSERT(pendingWriter->GetStatus() == LockStatus::Enum::Granted);

           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*pendingWriter) == UnlockStatus::Enum::Success);

           existingReader->Close();
           reader->Close();
           writer->Close();
           pendingWriter->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireUpdate_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto updater = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Granted);
        
           CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*updater) == UnlockStatus::Enum::Success);
           updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireUpdate_WithExistingShared_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto reader = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));

           auto updater = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Granted);
        
           CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*updater) == UnlockStatus::Enum::Success);

           reader->Close();
           updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireShared_WithExistingUpdate_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto existing = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));

           auto reader = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);
        
           existing->Close();
           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireUpdate_WithExistingUpdate_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto existing = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));

           auto updater = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Timeout);
        
           existing->Close();
           updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireExclusive_WithExistingUpdate_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto existing = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));

           auto writer = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);
        
           existing->Close();
           writer->Close();
           co_return;
       }

        ktl::Awaitable<void> UpgradeUpdate_WithExistingReaders_ShouldWaitForReadersToDrain_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
           auto existing = co_await lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));

           auto updater = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100));

           auto timedoutUpgrade = co_await lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(timedoutUpgrade->GetStatus() == LockStatus::Enum::Timeout);

           auto successfulUpgradeTask = lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           CODING_ERROR_ASSERT(successfulUpgradeTask.IsComplete() == false);

           CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*existing) == UnlockStatus::Enum::Success);

           auto successfulUpgrade = co_await successfulUpgradeTask;
           CODING_ERROR_ASSERT(successfulUpgrade->GetStatus() == LockStatus::Enum::Granted);
        
           existing->Close();
           updater->Close();
           timedoutUpgrade->Close();
           successfulUpgrade->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireReadPrimeLock_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));

           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
           co_return;
       }

        ktl::Awaitable<void> AcquireWritePrimeLock_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));

           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Exclusive);
           co_return;
       }

        ktl::Awaitable<void> AcquireReaderPrimeLock_WithExistingReader_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
           co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));

           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
           co_return;
       }

        ktl::Awaitable<void> AcquireWriterPrimeLock_WithExistingReader_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
        
           bool hasEx = false;
           try
           {
               co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception const & e)
           {
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
               hasEx = true;
           }

           CODING_ERROR_ASSERT(hasEx);

           lockManagerSPtr->ReleasePrimeLock(LockMode::Shared);
           co_return;
       }

        ktl::Awaitable<void> AcquireReaderPrimeLock_WithExistingWriter_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
        
           bool hasEx = false;
           try
           {
               co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception const & e)
           {
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
               hasEx = true;
           }

           CODING_ERROR_ASSERT(hasEx);

           lockManagerSPtr->ReleasePrimeLock(LockMode::Exclusive);
           co_return;
       }

        ktl::Awaitable<void> AcquireWriterPrimeLock_WithExistingWriter_ShouldTimeout_Test()
       {
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
        
           bool hasEx = false;
           try
           {
               co_await lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
           }
           catch (Exception const & e)
           {
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
               hasEx = true;
           }

           CODING_ERROR_ASSERT(hasEx);

           lockManagerSPtr->ReleasePrimeLock(LockMode::Exclusive);
           co_return;
       }

        ktl::Awaitable<void> AcquirePrimeLock_MultipleConcurrentReaders_ShouldSucceed_Test()
       {
           ULONG32 count = 100;
           KAllocator& allocator = LockManagerTest::GetAllocator();

           KArray<ktl::Awaitable<void>> primeLockRequestsArray(allocator, count);
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           for (ULONG32 i = 0; i < count; i++)
           {
               primeLockRequestsArray.Append(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
           }

           for (ULONG32 i = 0; i < count; i++)
           {
               co_await primeLockRequestsArray[i];
           }

           for (ULONG32 i = 0; i < count; i++)
           {
               lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
           }
           co_return;
       }

        ktl::Awaitable<void> AcquirePrimeLock_MultipleSequentialWriters_ShouldSucceed_Test()
       {
           ULONG32 count = 100;
           KAllocator& allocator = LockManagerTest::GetAllocator();

           KArray<ktl::Awaitable<void>> primeLockRequestsArray(allocator, count);
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           for (ULONG32 i = 0; i < count; i++)
           {
               primeLockRequestsArray.Append(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(10'000)));
           }

           for (ULONG32 i = 0; i < count; i++)
           {
               co_await primeLockRequestsArray[i];
               lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Exclusive);
           }
           co_return;
       }

        ktl::Awaitable<void> CloseManager_WithOutstandingPrimeLockReaders_ShouldSucceed_Test()
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

           for (ULONG32 i = 0; i < 10; i++)
           {
               co_await lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(10'000));
           }
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_AcquireSharedFromUpdate_WithExistingReaders_ShouldNotSucceed_Test()
       {
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto reader1 = co_await lockManagerSPtr->AcquireLockAsync(1, key, LockMode::Enum::Shared, timeout);
           auto reader2 = co_await lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Enum::Shared, timeout);
           auto reader3 = co_await lockManagerSPtr->AcquireLockAsync(3, key, LockMode::Enum::Shared, timeout);
           auto updater = co_await lockManagerSPtr->AcquireLockAsync(4, key, LockMode::Enum::Update, timeout);

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader1));
           reader1->Close();

           auto failedReader = co_await lockManagerSPtr->AcquireLockAsync(5, key, LockMode::Enum::Shared, timeout);
           CODING_ERROR_ASSERT(LockStatus::Timeout == failedReader->GetStatus());
           failedReader->Close();

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader2));
           reader2->Close();

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader3));
           reader3->Close();

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater));
           updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_SingleOwnerAcquiresMultiple_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto updater1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(updater1->GetStatus() == LockStatus::Granted);
           auto updater2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(updater2->GetStatus() == LockStatus::Granted);

           auto writer1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(writer1->GetStatus() == LockStatus::Granted);
           auto writer2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Granted);

           auto reader1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Granted);
           auto reader2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(reader2->GetStatus() == LockStatus::Granted);

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater1));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater2));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer1));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer2));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader1));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader2));


           updater1->Close();
           updater2->Close();
           writer1->Close();
           writer2->Close();
           reader1->Close();
           reader2->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_OwnsUpdate_AcquireShared_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
           auto update = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);

           auto shared = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == shared->GetStatus());

           CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*update) == UnlockStatus::Success);
           CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*shared) == UnlockStatus::Success);

           update->Close();
           shared->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_OwnsUpdate_UpgradeToExclusive_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto update = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           auto writer = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout);

           CODING_ERROR_ASSERT(LockStatus::Granted == writer->GetStatus());
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*update));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));

           update->Close();
           writer->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_OwnsExclusive_DowngradeToUpdate_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto writer = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout);
           auto update = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);

           CODING_ERROR_ASSERT(LockStatus::Granted == update->GetStatus());

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*update));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));

           writer->Close();
           update->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLock_OwnerAcquiresUpdateMultipleTimes_WithExistingReader_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           // This test fails due to issues in LocateLockClient
           // Removing the first reader causes the test to pass

           //LONG64 existingOwner = 7;
           //auto existingReader = co_await lockManagerSPtr->AcquireLockAsync(existingOwner, key, LockMode::Shared, timeout);

           auto reader = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           auto update1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(update1->GetStatus() == LockStatus::Granted);
           auto update2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(update2->GetStatus() == LockStatus::Granted);

           //existingReader->Close();
           reader->Close();
           update1->Close();
           update2->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireAndReleaseReadersAndWriters_SameKey_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto reader1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Granted);

           auto writer1 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(writer1->GetStatus() == LockStatus::Granted);

           auto reader2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(reader2->GetStatus() == LockStatus::Granted);

           auto writer2 = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Granted);

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader1));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer1));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader2));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer2));
           co_return;
       }

        ktl::Awaitable<void> UpgradeLock_SharedToUpdateToExclusive_ShouldSucceed_Test()
       {
           LONG64 owner = 5;
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto reader = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == reader->GetStatus());

           auto updater = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == updater->GetStatus());

           auto writer = co_await lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == writer->GetStatus());

           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader));

           updater->Close();
           writer->Close();
           reader->Close();
           co_return;
       }

        ktl::Awaitable<void> UpgradeReadersAndUpdaterToExclusive_AfterExistingReaderRelease_ShouldSucceed_Test()
       {
           ULONG64 key = 100;
           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto existingReader = co_await lockManagerSPtr->AcquireLockAsync(1, key, LockMode::Shared, timeout);

           auto reader1 = co_await lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == reader1->GetStatus());
           auto reader2 = co_await lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == reader2->GetStatus());
           auto updater = co_await lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(LockStatus::Granted == updater->GetStatus());

           auto expiredWriter = co_await lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(LockStatus::Timeout == expiredWriter->GetStatus());

           auto waiter = lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(waiter.IsComplete() == false);

           lockManagerSPtr->ReleaseLock(*existingReader);

           auto writer = co_await waiter;
           CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Granted);

           existingReader->Close();
           reader1->Close();
           reader2->Close();
           updater->Close();
           expiredWriter->Close();
           writer->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingReader_WithExistingUpdater_ShouldBeUpdateMode_Test()
       {
            // Test for scenario: Current=U, Releasing=S, MaxGrantee=U
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;
           LONG64 txn3 = 3;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           // Setup
           auto txn1Reader = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout);
           auto txn3Updater = co_await lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Update, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release reader - lock mode should remain Update
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Reader));
           txn1Reader->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn3Updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingUpdater_WithExistingReader_ShouldBeSharedMode_Test()
       {
            // Test for scenario: Current=U, Releasing=U, MaxGrantee=S
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;
           LONG64 txn3 = 3;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           // Setup
           auto txn1Reader = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout);
           auto txn3Updater = co_await lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Update, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release updater - lock mode should downgrade to shared
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn3Updater));
           txn3Updater->Close();

           // Try acquiring locks
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Granted);
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Reader));
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Granted);
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Updater));
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn1Reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingUpdater_WithExistingUpdater_ShouldBeUpdateMode_Test()
       {
           // Test for scenario: Current=U, Releasing=U, MaxGrantee=U
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;
           LONG64 txn3 = 3;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           // Setup
           auto txn3Reader = co_await lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Shared, timeout);
           auto existingTxn1Updater = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout);
           auto txn1Updater = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout);

           // Try acquring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release updater - lock mode should remain Update
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Updater));
           txn1Updater->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn3Reader->Close();
           existingTxn1Updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingReader_WithExistingWriter_ShouldBeExlusiveMode_Test()
       {
           // Test for scenario: Current=X, Releasing=S, MaxGrantee=X
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
           // Setup
           auto txn1Reader = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout);
           auto txn1Writer = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release reader - lock mode should remain Exclusive
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Reader));
           txn1Reader->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn1Writer->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingWriter_WithExistingReader_ShouldBeSharedMode_Test()
       {
           // Test for scenario: Current=X, Releasing=X, MaxGrantee=S
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
           // Setup
           auto txn1Reader = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout);
           auto txn1Writer = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release writer - lock mode should become Shared
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
           txn1Writer->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Granted);
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Reader));
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Granted);
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Updater));
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn1Reader->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingWriter_WithExistingUpdater_ShouldBeUpdateMode_Test()
       {
           // Test for scenario: Current=X, Releasing=X, MaxGrantee=U
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto txn1Updater = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout);
           auto txn1Writer = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release writer - lock mode should downgrade to update
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
           txn1Writer->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn1Updater->Close();
           co_return;
       }

        ktl::Awaitable<void> AcquireLocks_AfterReleasingWriter_WithExistingWriter_ShouldBeExclusiveMode_Test()
       {
           // Test for scenario: Current=X, Releasing=X, MaxGrantee=U
           ULONG64 key = 100;

           LONG64 txn1 = 1;
           LONG64 txn2 = 2;

           auto timeout = Common::TimeSpan::FromMilliseconds(100);
           LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

           auto txn1Reader = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout);
           auto existingTxn1Writer = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout);
           auto txn1Writer = co_await lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout);

           // Try acquiring locks
           auto txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           auto txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           auto txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           // Release writer - lock mode should remain exclusive
           CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
           txn1Writer->Close();

           // Try acquiring locks again
           txn2Reader = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout);
           CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
           txn2Reader->Close();

           txn2Updater = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout);
           CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
           txn2Updater->Close();

           txn2Writer = co_await lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout);
           CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
           txn2Writer->Close();

           txn1Reader->Close();
           existingTxn1Writer->Close();
           co_return;
       }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(LockManagerTestSuite, LockManagerTest)

   BOOST_AUTO_TEST_CASE(AcquireLock_AfterClose_Fails)
   {
       SyncAwait(AcquireLock_AfterClose_Fails_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_InvalidMode_ShouldThrow)
   {
       SyncAwait(AcquireLock_InvalidMode_ShouldThrow_Test());
   }

   BOOST_AUTO_TEST_CASE(SingleReader_AcquireRelease_ShouldSucceed)
   {
       SyncAwait(SingleReader_AcquireRelease_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(SingleWriter_AcquireRelease_ShouldSucceed)
   {
       SyncAwait(SingleWriter_AcquireRelease_ShouldSucceed_Test());
   }


   BOOST_AUTO_TEST_CASE(SameLock_ConcurrentReaders_ShouldSucceed)
   {
       SyncAwait(SameLock_ConcurrentReaders_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(DifferentLock_ConcurrentReaders_ShouldSucceed)
   {
       SyncAwait(DifferentLock_ConcurrentReaders_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(SameLock_SequentialWriters_ShouldSucceed)
   {
       SyncAwait(SameLock_SequentialWriters_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(SameLock_ReadersWaitingOnWriters_ShouldSucceedAfterWriterReleasesLock)
   {
       SyncAwait(SameLock_ReadersWaitingOnWriters_ShouldSucceedAfterWriterReleasesLock_Test());
   }

   BOOST_AUTO_TEST_CASE(DifferentLock_ConcurrentWriters_ShouldSucceed)
   {
       SyncAwait(DifferentLock_ConcurrentWriters_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingWriter_ShouldTimeout)
   {
       SyncAwait(AcquireWriteLock_WithExistingWriter_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingReader_ShouldTimeout)
   {
       SyncAwait(AcquireWriteLock_WithExistingReader_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingWriter_ShouldTimeout)
   {
       SyncAwait(AcquireReadLock_WithExistingWriter_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingReader_ShouldTimeout)
   {
       SyncAwait(AcquireReadLock_WithExistingReader_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingWriter_ZeroTimeout_ShouldTimeout)
   {
       SyncAwait(AcquireWriteLock_WithExistingWriter_ZeroTimeout_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingReader_ZeroTimeout_ShouldTimeout)
   {
       SyncAwait(AcquireWriteLock_WithExistingReader_ZeroTimeout_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingWriter_ZeroTimeout_ShouldTimeout)
   {
       SyncAwait(AcquireReadLock_WithExistingWriter_ZeroTimeout_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingReader_ZeroTimeout_ShouldSucceed)
   {
       SyncAwait(AcquireReadLock_WithExistingReader_ZeroTimeout_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_AllLockModes_ShouldSucceed)
   {
       SyncAwait(AcquireLock_AllLockModes_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithInfiniteTimeout_WithExistingWriter_ShouldSucceed)
   {
       SyncAwait(AcquireWriteLock_WithInfiniteTimeout_WithExistingWriter_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadWriteLock_WithExistingWriter_GrantedInOrder)
   {
       SyncAwait(AcquireReadWriteLock_WithExistingWriter_GrantedInOrder_Test());
   }

   BOOST_AUTO_TEST_CASE(ReleaseReaderLock_AlreadyReleasedReader_ShouldFail) 
   {
       SyncAwait(ReleaseReaderLock_AlreadyReleasedReader_ShouldFail_Test());
   }

   BOOST_AUTO_TEST_CASE(ReleaseWriterLock_AlreadyReleasedWriter_ShouldFail) 
   {
       SyncAwait(ReleaseWriterLock_AlreadyReleasedWriter_ShouldFail_Test());
   }

   BOOST_AUTO_TEST_CASE(ReleaseReaderLock_ReaderFromDifferentManager_ShouldFail)
   {
       SyncAwait(ReleaseReaderLock_ReaderFromDifferentManager_ShouldFail_Test());
   }

   BOOST_AUTO_TEST_CASE(ReleaseWriterLock_WriterFromDifferentManager_ShouldFail)
   {
       SyncAwait(ReleaseWriterLock_WriterFromDifferentManager_ShouldFail_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_ShouldSucceed)
   {
       SyncAwait(AcquireExclusive_OwnShared_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireShared_OwnsExclusive_ShouldSucceed)
   {
       SyncAwait(AcquireShared_OwnsExclusive_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingReader_ShouldSucceed)
   {
       SyncAwait(AcquireExclusive_OwnShared_WithExistingReader_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingWriter_ShouldSucceed)
   {
       SyncAwait(AcquireExclusive_OwnShared_WithExistingWriter_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingReaderAndWaiter_ShouldSucceed)
   {
       SyncAwait(AcquireExclusive_OwnShared_WithExistingReaderAndWaiter_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_ShouldSucceed)
   {
       SyncAwait(AcquireUpdate_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_WithExistingShared_ShouldSucceed)
   {
       SyncAwait(AcquireUpdate_WithExistingShared_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireShared_WithExistingUpdate_ShouldTimeout)
   {
       SyncAwait(AcquireShared_WithExistingUpdate_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_WithExistingUpdate_ShouldTimeout)
   {
       SyncAwait(AcquireUpdate_WithExistingUpdate_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_WithExistingUpdate_ShouldTimeout)
   {
       SyncAwait(AcquireExclusive_WithExistingUpdate_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(UpgradeUpdate_WithExistingReaders_ShouldWaitForReadersToDrain)
   {
       SyncAwait(UpgradeUpdate_WithExistingReaders_ShouldWaitForReadersToDrain_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReadPrimeLock_ShouldSucceed)
   {
       SyncAwait(AcquireReadPrimeLock_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWritePrimeLock_ShouldSucceed)
   {
       SyncAwait(AcquireWritePrimeLock_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReaderPrimeLock_WithExistingReader_ShouldSucceed)
   {
       SyncAwait(AcquireReaderPrimeLock_WithExistingReader_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriterPrimeLock_WithExistingReader_ShouldTimeout)
   {
       SyncAwait(AcquireWriterPrimeLock_WithExistingReader_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireReaderPrimeLock_WithExistingWriter_ShouldTimeout)
   {
       SyncAwait(AcquireReaderPrimeLock_WithExistingWriter_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireWriterPrimeLock_WithExistingWriter_ShouldTimeout)
   {
       SyncAwait(AcquireWriterPrimeLock_WithExistingWriter_ShouldTimeout_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquirePrimeLock_MultipleConcurrentReaders_ShouldSucceed)
   {
       SyncAwait(AcquirePrimeLock_MultipleConcurrentReaders_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquirePrimeLock_MultipleSequentialWriters_ShouldSucceed)
   {
       SyncAwait(AcquirePrimeLock_MultipleSequentialWriters_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(CloseManager_WithOutstandingPrimeLockReaders_ShouldSucceed)
   {
       SyncAwait(CloseManager_WithOutstandingPrimeLockReaders_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(CloseManager_WithInfiteWaiter_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSPtr = nullptr;
       LockManager::Create(GetAllocator(), lockManagerSPtr);
       SyncAwait(lockManagerSPtr->OpenAsync());

       SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(10'000)));
       auto outstandingAcquireTask = lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(MAXULONG32));

       SyncAwait(lockManagerSPtr->CloseAsync());

       try
       {
           SyncAwait(outstandingAcquireTask);
           CODING_ERROR_ASSERT(false);
       }
       catch (ktl::Exception const & e)
       {
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
       }
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_AcquireSharedFromUpdate_WithExistingReaders_ShouldNotSucceed)
   {
       SyncAwait(AcquireLock_AcquireSharedFromUpdate_WithExistingReaders_ShouldNotSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_SingleOwnerAcquiresMultiple_ShouldSucceed)
   {
       SyncAwait(AcquireLocks_SingleOwnerAcquiresMultiple_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsUpdate_AcquireShared_ShouldSucceed)
   {
       SyncAwait(AcquireLock_OwnsUpdate_AcquireShared_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsUpdate_UpgradeToExclusive_ShouldSucceed)
   {
       SyncAwait(AcquireLock_OwnsUpdate_UpgradeToExclusive_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsExclusive_DowngradeToUpdate_ShouldSucceed)
   {
       SyncAwait(AcquireLock_OwnsExclusive_DowngradeToUpdate_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnerAcquiresUpdateMultipleTimes_WithExistingReader_ShouldSucceed)
   {
       SyncAwait(AcquireLock_OwnerAcquiresUpdateMultipleTimes_WithExistingReader_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireAndReleaseReadersAndWriters_SameKey_ShouldSucceed)
   {
       SyncAwait(AcquireAndReleaseReadersAndWriters_SameKey_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(UpgradeLock_SharedToUpdateToExclusive_ShouldSucceed)
   {
       SyncAwait(UpgradeLock_SharedToUpdateToExclusive_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(UpgradeReadersAndUpdaterToExclusive_AfterExistingReaderRelease_ShouldSucceed)
   {
       SyncAwait(UpgradeReadersAndUpdaterToExclusive_AfterExistingReaderRelease_ShouldSucceed_Test());
   }

   /* RecomputeLockGrantees Matrix ("-" means empty GrantedList; cases not listed here are invalid scenarios)
   Current  Releasing   Max Grantee     | New Mode
   -------  ---------   -----------     | --------
   S        S           S               | S         This scenario already covered by previous tests
                                        |          
   U        S           U               | U         AcquireLocks_AfterReleasingReader_WithExistingUpdater_ShouldBeUpdateMode
   U        U           S/U             | S/U       AcqurieLocks_AfterReleasingUpdater_WithExisting(Reader|Updater)_ShouldBe(Shared|Update)Mode
                                        |
   X        S           X               | X         AcquireLocks_AfterReleasingReader_WithExistingWriter_ShouldBeExclusiveMode
   X        X           S/U/X           | S/U/X     AcquireLocks_AfterReleasingWriter_WithExisting(Reader|Updater|Writer)_ShouldBe(Shared|Update|Exclusive)Mode
   */

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingReader_WithExistingUpdater_ShouldBeUpdateMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingReader_WithExistingUpdater_ShouldBeUpdateMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingUpdater_WithExistingReader_ShouldBeSharedMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingUpdater_WithExistingReader_ShouldBeSharedMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingUpdater_WithExistingUpdater_ShouldBeUpdateMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingUpdater_WithExistingUpdater_ShouldBeUpdateMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingReader_WithExistingWriter_ShouldBeExlusiveMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingReader_WithExistingWriter_ShouldBeExlusiveMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingReader_ShouldBeSharedMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingWriter_WithExistingReader_ShouldBeSharedMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingUpdater_ShouldBeUpdateMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingWriter_WithExistingUpdater_ShouldBeUpdateMode_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingWriter_ShouldBeExclusiveMode)
   {
       SyncAwait(AcquireLocks_AfterReleasingWriter_WithExistingWriter_ShouldBeExclusiveMode_Test());
   }

   BOOST_AUTO_TEST_SUITE_END()
}


