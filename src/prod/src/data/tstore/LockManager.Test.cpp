// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
   using namespace Common;
   using namespace ktl;
   using namespace Data::TStore;
    using namespace TxnReplicator;

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
            lockManagerSPtr_->Close();
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
         lockManagerSPtr_->Open();
         return lockManagerSPtr_;
      }

   private:
      KtlSystem* ktlSystem_;
      LockManager::SPtr lockManagerSPtr_;
   };

   BOOST_FIXTURE_TEST_SUITE(LockManagerTestSuite, LockManagerTest)

   BOOST_AUTO_TEST_CASE(AcquireLock_AfterClose_Fails)
   {
       LockManager::SPtr lockManagerSptr;
       auto status = LockManager::Create(LockManagerTest::GetAllocator(), lockManagerSptr);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       lockManagerSptr->Open();
       lockManagerSptr->Close();

       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
       }
       catch (Exception & e)
       {
           hasEx = true;
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
       }

       CODING_ERROR_ASSERT(hasEx);

       try
       {
           SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
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
           auto lock = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 0, mode, TimeSpan::FromMilliseconds(100)));
           CODING_ERROR_ASSERT(lock->GetStatus() == LockStatus::Enum::Invalid);
           lock->Close();
       }
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_InvalidMode_ShouldThrow)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSptr->AcquireLockAsync(17, 0, LockMode::Enum::Free, TimeSpan::FromMilliseconds(100)));
       }
       catch (Exception & e)
       {
           hasEx = true;
           CODING_ERROR_ASSERT(e.GetStatus() == STATUS_INVALID_PARAMETER_3);
       }

       CODING_ERROR_ASSERT(hasEx);
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_InvalidTimeout_ShouldThrow)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSptr->AcquireLockAsync(17, 0, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(-100)));
       }
       catch (Exception & e)
       {
           hasEx = true;
           CODING_ERROR_ASSERT(e.GetStatus() == STATUS_INVALID_PARAMETER_4);
       }

       CODING_ERROR_ASSERT(hasEx);
   }

   BOOST_AUTO_TEST_CASE(SingleReader_AcquireRelease_ShouldSucceed)
   {
      LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

      auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

      auto status = lockManagerSptr->ReleaseLock(*reader);
      CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

      reader->Close();
   }

   BOOST_AUTO_TEST_CASE(SingleWriter_AcquireRelease_ShouldSucceed)
   {
      LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

      auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

      auto status = lockManagerSptr->ReleaseLock(*writer);
      CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

      writer->Close();
   }


   BOOST_AUTO_TEST_CASE(SameLock_ConcurrentReaders_ShouldSucceed)
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
         LockControlBlock::SPtr reader = SyncAwait(locksAwaitableArray[i]);
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
   }

   BOOST_AUTO_TEST_CASE(DifferentLock_ConcurrentReaders_ShouldSucceed)
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
         LockControlBlock::SPtr reader = SyncAwait(locksAwaitableArray[i]);
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
   }

   BOOST_AUTO_TEST_CASE(SameLock_SequentialWriters_ShouldSucceed)
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
         LockControlBlock::SPtr writer = SyncAwait(locksAwaitableArray[i]);
         CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
         auto status = lockManagerSptr->ReleaseLock(*writer);
         CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

         writer->Close();
      }
   }

   BOOST_AUTO_TEST_CASE(SameLock_ReadersWaitingOnWriters_ShouldSucceedAfterWriterReleasesLock)
   {
       ULONG32 count = 2;
       KAllocator& allocator = GetAllocator();

       KArray<ktl::Awaitable<KSharedPtr<LockControlBlock>>> locksAwaitableArray(allocator, count);
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(10000, 100, LockMode::Enum::Exclusive, TimeSpan::MaxValue));

       for (ULONG32 i = 0; i < count; i++)
       {
           ktl::Awaitable<KSharedPtr<LockControlBlock>> reader = lockManagerSptr->AcquireLockAsync(i, 100, LockMode::Enum::Shared, TimeSpan::MaxValue);
           locksAwaitableArray.Append(Ktl::Move(reader));
       }

       lockManagerSptr->ReleaseLock(*writer);
       //writer->Close();

       for (ULONG32 i = 0; i < count; i++)
       {
           LockControlBlock::SPtr reader = SyncAwait(locksAwaitableArray[i]);
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
           //reader->Close();
       }

       // Close with the granted readers.
   }

   BOOST_AUTO_TEST_CASE(DifferentLock_ConcurrentWriters_ShouldSucceed)
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
         LockControlBlock::SPtr writer = SyncAwait(locksAwaitableArray[i]);
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
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingWriter_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

       auto blockedWriter = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(blockedWriter->GetStatus() == LockStatus::Enum::Timeout);

       auto status = lockManagerSptr->ReleaseLock(*writer);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       writer->Close();
       blockedWriter->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingReader_ShouldTimeout)
   {
      LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

      auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

      auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);

      auto status = lockManagerSptr->ReleaseLock(*reader);
      CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

      writer->Close();
      reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingWriter_ShouldTimeout)
   {
      LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

      auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

      auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
      CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);

      auto status = lockManagerSptr->ReleaseLock(*writer);
      CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

      writer->Close();
      reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingReader_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

       auto blockedReader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(blockedReader->GetStatus() == LockStatus::Enum::Timeout);

       auto status = lockManagerSptr->ReleaseLock(*reader);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       reader->Close();
       blockedReader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingWriter_ZeroTimeout_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::Zero));
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

       auto blockedWriter = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::Zero));
       CODING_ERROR_ASSERT(blockedWriter->GetStatus() == LockStatus::Enum::Timeout);

       auto status = lockManagerSptr->ReleaseLock(*writer);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       writer->Close();
       blockedWriter->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireWriteLock_WithExistingReader_ZeroTimeout_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::Zero));
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::Zero));
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);

       auto status = lockManagerSptr->ReleaseLock(*reader);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       writer->Close();
       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingWriter_ZeroTimeout_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::Zero));
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::Zero));
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);

       auto status = lockManagerSptr->ReleaseLock(*writer);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       writer->Close();
       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireReadLock_WithExistingReader_ZeroTimeout_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto existingReader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::Zero));
       CODING_ERROR_ASSERT(existingReader->GetStatus() == LockStatus::Enum::Granted);

       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::Zero));
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

       auto status = lockManagerSptr->ReleaseLock(*existingReader);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       status = lockManagerSptr->ReleaseLock(*reader);
       CODING_ERROR_ASSERT(status == UnlockStatus::Enum::Success);

       existingReader->Close();
       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_AllLockModes_ShouldSucceed)
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
               auto lock = SyncAwait(locksAwaitableArray[i]);
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
   }

   BOOST_AUTO_TEST_CASE(AcquireReadWriteLock_WithExistingWriter_GrantedInOrder)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));

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
       auto reader1 = SyncAwait(reader1Task);
       CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(writer2Task.IsComplete() == false);
       CODING_ERROR_ASSERT(reader3Task.IsComplete() == false);
       CODING_ERROR_ASSERT(reader4Task.IsComplete() == false);
       CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
       CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader1) == UnlockStatus::Enum::Success);

       // writer2
       auto writer2 = SyncAwait(writer2Task);
       CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(reader3Task.IsComplete() == false);
       CODING_ERROR_ASSERT(reader4Task.IsComplete() == false);
       CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
       CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer2) == UnlockStatus::Enum::Success);

       // reader3 and reader4
       auto reader3 = SyncAwait(reader3Task);
       CODING_ERROR_ASSERT(reader3->GetStatus() == LockStatus::Enum::Granted);
       auto reader4 = SyncAwait(reader4Task);
       CODING_ERROR_ASSERT(reader4->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(writer5Task.IsComplete() == false);
       CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader3) == UnlockStatus::Enum::Success);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader4) == UnlockStatus::Enum::Success);

       // writer5
       auto writer5 = SyncAwait(writer5Task);
       CODING_ERROR_ASSERT(writer5->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(writer6Task.IsComplete() == false);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer5) == UnlockStatus::Enum::Success);

       // writer6
       auto writer6 = SyncAwait(writer6Task);
       CODING_ERROR_ASSERT(writer6->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer6) == UnlockStatus::Enum::Success);

       writer->Close();
       reader1->Close();
       writer2->Close();
       reader3->Close();
       reader4->Close();
       writer5->Close();
       writer6->Close();
   }

   BOOST_AUTO_TEST_CASE(ReleaseReaderLock_AlreadyReleasedReader_ShouldFail) 
   {
      LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
      auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));
      lockManagerSptr->ReleaseLock(*reader);

      CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::NotGranted);

      reader->Close();
   }

   BOOST_AUTO_TEST_CASE(ReleaseWriterLock_AlreadyReleasedWriter_ShouldFail) 
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000)));
       lockManagerSptr->ReleaseLock(*writer);

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::NotGranted);

       writer->Close();
   }

   BOOST_AUTO_TEST_CASE(ReleaseReaderLock_ReaderFromDifferentManager_ShouldFail)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       LockManager::SPtr otherManagerSptr;
       auto status = LockManager::Create(LockManagerTest::GetAllocator(), otherManagerSptr);
       otherManagerSptr->Open();
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       auto otherReader = SyncAwait(otherManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*otherReader) == UnlockStatus::Enum::UnknownResource);
       CODING_ERROR_ASSERT(otherManagerSptr->ReleaseLock(*otherReader) == UnlockStatus::Enum::Success);

       otherReader->Close();

       otherManagerSptr->Close();
   }

   BOOST_AUTO_TEST_CASE(ReleaseWriterLock_WriterFromDifferentManager_ShouldFail)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       LockManager::SPtr otherManagerSptr;
       auto status = LockManager::Create(LockManagerTest::GetAllocator(), otherManagerSptr);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));
       otherManagerSptr->Open();

       auto otherWriter = SyncAwait(otherManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000)));

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*otherWriter) == UnlockStatus::Enum::UnknownResource);
       CODING_ERROR_ASSERT(otherManagerSptr->ReleaseLock(*otherWriter) == UnlockStatus::Enum::Success);

       otherWriter->Close();

       otherManagerSptr->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));
       auto upgradedWriter = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000)));

       CODING_ERROR_ASSERT(upgradedWriter->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*upgradedWriter) == UnlockStatus::Enum::Success);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

       upgradedWriter->Close();

       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireShared_OwnsExclusive_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000)));
       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));

       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);

       reader->Close();
       writer->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingReader_ShouldSucceed)
   {
       KAllocator& allocator = GetAllocator();
       try
       {
           LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
           auto existingReader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));

           auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));
           auto writerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

           // Reader should get in immediately, writer will have to wait until existingReader is finished
           CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);
           CODING_ERROR_ASSERT(writerRequest.IsComplete() == false);

           // Release existing reader
           CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*existingReader) == UnlockStatus::Enum::Success);

           // Writer should be able to get in now
           auto writer = SyncAwait(writerRequest);
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
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingWriter_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();
       auto existingWriter = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));

       auto readerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000));
       auto writerRequest = lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(1000));

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*existingWriter) == UnlockStatus::Enum::Success);

       auto reader = SyncAwait(readerRequest);
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Granted);

       auto writer = SyncAwait(writerRequest);
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*reader) == UnlockStatus::Enum::Success);

       writer->Close();
       reader->Close();
       existingWriter->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_OwnShared_WithExistingReaderAndWaiter_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       auto existingReader = SyncAwait(lockManagerSptr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));

       // Acquire a read lock, add a pending write request, then upgrade read lock to a write lock
       auto reader = SyncAwait(lockManagerSptr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(1000)));
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
       auto writer = SyncAwait(writerRequest);
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Granted);
       CODING_ERROR_ASSERT(pendingWriterRequest.IsComplete() == false);

       // pendingWriterRequest should be granted next
       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*writer) == UnlockStatus::Enum::Success);
       auto pendingWriter = SyncAwait(pendingWriterRequest);
       CODING_ERROR_ASSERT(pendingWriter->GetStatus() == LockStatus::Enum::Granted);

       CODING_ERROR_ASSERT(lockManagerSptr->ReleaseLock(*pendingWriter) == UnlockStatus::Enum::Success);

       existingReader->Close();
       reader->Close();
       writer->Close();
       pendingWriter->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Granted);
        
       CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*updater) == UnlockStatus::Enum::Success);
       updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_WithExistingShared_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));

       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Granted);
        
       CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*updater) == UnlockStatus::Enum::Success);

       reader->Close();
       updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireShared_WithExistingUpdate_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto existing = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));

       auto reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(reader->GetStatus() == LockStatus::Enum::Timeout);
        
       existing->Close();
       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireUpdate_WithExistingUpdate_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto existing = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));

       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(updater->GetStatus() == LockStatus::Enum::Timeout);
        
       existing->Close();
       updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireExclusive_WithExistingUpdate_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto existing = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));

       auto writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Enum::Timeout);
        
       existing->Close();
       writer->Close();
   }

   BOOST_AUTO_TEST_CASE(UpgradeUpdate_WithExistingReaders_ShouldWaitForReadersToDrain)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
       auto existing = SyncAwait(lockManagerSPtr->AcquireLockAsync(17, 100, LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));

       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Update, TimeSpan::FromMilliseconds(100)));

       auto timedoutUpgrade = SyncAwait(lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       CODING_ERROR_ASSERT(timedoutUpgrade->GetStatus() == LockStatus::Enum::Timeout);

       auto successfulUpgradeTask = lockManagerSPtr->AcquireLockAsync(18, 100, LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100));
       CODING_ERROR_ASSERT(successfulUpgradeTask.IsComplete() == false);

       CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*existing) == UnlockStatus::Enum::Success);

       auto successfulUpgrade = SyncAwait(successfulUpgradeTask);
       CODING_ERROR_ASSERT(successfulUpgrade->GetStatus() == LockStatus::Enum::Granted);
        
       existing->Close();
       updater->Close();
       timedoutUpgrade->Close();
       successfulUpgrade->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireReadPrimeLock_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));

       lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
   }

   BOOST_AUTO_TEST_CASE(AcquireWritePrimeLock_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));

       lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Exclusive);
   }

   BOOST_AUTO_TEST_CASE(AcquireReaderPrimeLock_WithExistingReader_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
       SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));

       lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
       lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
   }

   BOOST_AUTO_TEST_CASE(AcquireWriterPrimeLock_WithExistingReader_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
        
       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       }
       catch (Exception const & e)
       {
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
           hasEx = true;
       }

       CODING_ERROR_ASSERT(hasEx);

       lockManagerSPtr->ReleasePrimeLock(LockMode::Shared);
   }

   BOOST_AUTO_TEST_CASE(AcquireReaderPrimeLock_WithExistingWriter_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
        
       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(100)));
       }
       catch (Exception const & e)
       {
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
           hasEx = true;
       }

       CODING_ERROR_ASSERT(hasEx);

       lockManagerSPtr->ReleasePrimeLock(LockMode::Exclusive);
   }

   BOOST_AUTO_TEST_CASE(AcquireWriterPrimeLock_WithExistingWriter_ShouldTimeout)
   {
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
        
       bool hasEx = false;
       try
       {
           SyncAwait(lockManagerSPtr->AcquirePrimeLockAsync(LockMode::Enum::Exclusive, TimeSpan::FromMilliseconds(100)));
       }
       catch (Exception const & e)
       {
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
           hasEx = true;
       }

       CODING_ERROR_ASSERT(hasEx);

       lockManagerSPtr->ReleasePrimeLock(LockMode::Exclusive);
   }

   BOOST_AUTO_TEST_CASE(AcquirePrimeLock_MultipleConcurrentReaders_ShouldSucceed)
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
           SyncAwait(primeLockRequestsArray[i]);
       }

       for (ULONG32 i = 0; i < count; i++)
       {
           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Shared);
       }
   }

   BOOST_AUTO_TEST_CASE(AcquirePrimeLock_MultipleSequentialWriters_ShouldSucceed)
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
           SyncAwait(primeLockRequestsArray[i]);
           lockManagerSptr->ReleasePrimeLock(LockMode::Enum::Exclusive);
       }
   }

   BOOST_AUTO_TEST_CASE(CloseManager_WithOutstandingPrimeLockReaders_ShouldSucceed)
   {
       LockManager::SPtr lockManagerSptr = LockManagerTest::CreateLockManager();

       for (ULONG32 i = 0; i < 10; i++)
       {
           SyncAwait(lockManagerSptr->AcquirePrimeLockAsync(LockMode::Enum::Shared, TimeSpan::FromMilliseconds(10'000)));
       }
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_AcquireSharedFromUpdate_WithExistingReaders_ShouldNotSucceed)
   {
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto reader1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(1, key, LockMode::Enum::Shared, timeout));
       auto reader2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Enum::Shared, timeout));
       auto reader3 = SyncAwait(lockManagerSPtr->AcquireLockAsync(3, key, LockMode::Enum::Shared, timeout));
       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(4, key, LockMode::Enum::Update, timeout));

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader1));
       reader1->Close();

       auto failedReader = SyncAwait(lockManagerSPtr->AcquireLockAsync(5, key, LockMode::Enum::Shared, timeout));
       CODING_ERROR_ASSERT(LockStatus::Timeout == failedReader->GetStatus());
       failedReader->Close();

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader2));
       reader2->Close();

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader3));
       reader3->Close();

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater));
       updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_SingleOwnerAcquiresMultiple_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto updater1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(updater1->GetStatus() == LockStatus::Granted);
       auto updater2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(updater2->GetStatus() == LockStatus::Granted);

       auto writer1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(writer1->GetStatus() == LockStatus::Granted);
       auto writer2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Granted);

       auto reader1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Granted);
       auto reader2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
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
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsUpdate_AcquireShared_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
       auto update = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));

       auto shared = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == shared->GetStatus());

       CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*update) == UnlockStatus::Success);
       CODING_ERROR_ASSERT(lockManagerSPtr->ReleaseLock(*shared) == UnlockStatus::Success);

       update->Close();
       shared->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsUpdate_UpgradeToExclusive_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto update = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       auto writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout));

       CODING_ERROR_ASSERT(LockStatus::Granted == writer->GetStatus());
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*update));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));

       update->Close();
       writer->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnsExclusive_DowngradeToUpdate_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout));
       auto update = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));

       CODING_ERROR_ASSERT(LockStatus::Granted == update->GetStatus());

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*update));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));

       writer->Close();
       update->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLock_OwnerAcquiresUpdateMultipleTimes_WithExistingReader_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       // This test fails due to issues in LocateLockClient
       // Removing the first reader causes the test to pass

       //LONG64 existingOwner = 7;
       //auto existingReader = SyncAwait(lockManagerSPtr->AcquireLockAsync(existingOwner, key, LockMode::Shared, timeout));

       auto reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       auto update1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(update1->GetStatus() == LockStatus::Granted);
       auto update2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(update2->GetStatus() == LockStatus::Granted);

       //existingReader->Close();
       reader->Close();
       update1->Close();
       update2->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireAndReleaseReadersAndWriters_SameKey_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto reader1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(reader1->GetStatus() == LockStatus::Granted);

       auto writer1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(writer1->GetStatus() == LockStatus::Granted);

       auto reader2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(reader2->GetStatus() == LockStatus::Granted);

       auto writer2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(writer2->GetStatus() == LockStatus::Granted);

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader1));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer1));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader2));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer2));
   }

   BOOST_AUTO_TEST_CASE(UpgradeLock_SharedToUpdateToExclusive_ShouldSucceed)
   {
       LONG64 owner = 5;
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == reader->GetStatus());

       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == updater->GetStatus());

       auto writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(owner, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == writer->GetStatus());

       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*updater));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*writer));
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*reader));

       updater->Close();
       writer->Close();
       reader->Close();
   }

   BOOST_AUTO_TEST_CASE(UpgradeReadersAndUpdaterToExclusive_AfterExistingReaderRelease_ShouldSucceed)
   {
       ULONG64 key = 100;
       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto existingReader = SyncAwait(lockManagerSPtr->AcquireLockAsync(1, key, LockMode::Shared, timeout));

       auto reader1 = SyncAwait(lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == reader1->GetStatus());
       auto reader2 = SyncAwait(lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == reader2->GetStatus());
       auto updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(LockStatus::Granted == updater->GetStatus());

       auto expiredWriter = SyncAwait(lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(LockStatus::Timeout == expiredWriter->GetStatus());

       auto waiter = lockManagerSPtr->AcquireLockAsync(2, key, LockMode::Exclusive, timeout);
       CODING_ERROR_ASSERT(waiter.IsComplete() == false);

       lockManagerSPtr->ReleaseLock(*existingReader);

       auto writer = SyncAwait(waiter);
       CODING_ERROR_ASSERT(writer->GetStatus() == LockStatus::Granted);

       existingReader->Close();
       reader1->Close();
       reader2->Close();
       updater->Close();
       expiredWriter->Close();
       writer->Close();
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
        // Test for scenario: Current=U, Releasing=S, MaxGrantee=U
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;
       LONG64 txn3 = 3;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       // Setup
       auto txn1Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout));
       auto txn3Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Update, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release reader - lock mode should remain Update
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Reader));
       txn1Reader->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn3Updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingUpdater_WithExistingReader_ShouldBeSharedMode)
   {
        // Test for scenario: Current=U, Releasing=U, MaxGrantee=S
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;
       LONG64 txn3 = 3;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       // Setup
       auto txn1Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout));
       auto txn3Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Update, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release updater - lock mode should downgrade to shared
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn3Updater));
       txn3Updater->Close();

       // Try acquiring locks
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Granted);
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Reader));
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Granted);
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Updater));
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn1Reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingUpdater_WithExistingUpdater_ShouldBeUpdateMode)
   {
       // Test for scenario: Current=U, Releasing=U, MaxGrantee=U
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;
       LONG64 txn3 = 3;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       // Setup
       auto txn3Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn3, key, LockMode::Shared, timeout));
       auto existingTxn1Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout));
       auto txn1Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout));

       // Try acquring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release updater - lock mode should remain Update
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Updater));
       txn1Updater->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn3Reader->Close();
       existingTxn1Updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingReader_WithExistingWriter_ShouldBeExlusiveMode)
   {
       // Test for scenario: Current=X, Releasing=S, MaxGrantee=X
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
       // Setup
       auto txn1Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout));
       auto txn1Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release reader - lock mode should remain Exclusive
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Reader));
       txn1Reader->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn1Writer->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingReader_ShouldBeSharedMode)
   {
       // Test for scenario: Current=X, Releasing=X, MaxGrantee=S
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();
        
       // Setup
       auto txn1Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout));
       auto txn1Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release writer - lock mode should become Shared
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
       txn1Writer->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Granted);
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Reader));
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Granted);
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn2Updater));
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn1Reader->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingUpdater_ShouldBeUpdateMode)
   {
       // Test for scenario: Current=X, Releasing=X, MaxGrantee=U
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto txn1Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Update, timeout));
       auto txn1Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release writer - lock mode should downgrade to update
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
       txn1Writer->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn1Updater->Close();
   }

   BOOST_AUTO_TEST_CASE(AcquireLocks_AfterReleasingWriter_WithExistingWriter_ShouldBeExclusiveMode)
   {
       // Test for scenario: Current=X, Releasing=X, MaxGrantee=U
       ULONG64 key = 100;

       LONG64 txn1 = 1;
       LONG64 txn2 = 2;

       auto timeout = Common::TimeSpan::FromMilliseconds(100);
       LockManager::SPtr lockManagerSPtr = LockManagerTest::CreateLockManager();

       auto txn1Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Shared, timeout));
       auto existingTxn1Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout));
       auto txn1Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn1, key, LockMode::Exclusive, timeout));

       // Try acquiring locks
       auto txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       auto txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       auto txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       // Release writer - lock mode should remain exclusive
       CODING_ERROR_ASSERT(UnlockStatus::Success == lockManagerSPtr->ReleaseLock(*txn1Writer));
       txn1Writer->Close();

       // Try acquiring locks again
       txn2Reader = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Shared, timeout));
       CODING_ERROR_ASSERT(txn2Reader->GetStatus() == LockStatus::Timeout);
       txn2Reader->Close();

       txn2Updater = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Update, timeout));
       CODING_ERROR_ASSERT(txn2Updater->GetStatus() == LockStatus::Timeout);
       txn2Updater->Close();

       txn2Writer = SyncAwait(lockManagerSPtr->AcquireLockAsync(txn2, key, LockMode::Exclusive, timeout));
       CODING_ERROR_ASSERT(txn2Writer->GetStatus() == LockStatus::Timeout);
       txn2Writer->Close();

       txn1Reader->Close();
       existingTxn1Writer->Close();
   }

   BOOST_AUTO_TEST_SUITE_END()
}


