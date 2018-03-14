// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsTP'

namespace TStoreTests
{
   using namespace ktl;

   class StoreTest3Replica : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
   {
   public:
      StoreTest3Replica()
      {
         Setup(3);
      }

      ~StoreTest3Replica()
      {
         Cleanup();
      }

      Common::CommonConfig config; // load the config object as its needed for the tracing to work
   };

   BOOST_FIXTURE_TEST_SUITE(StoreTest3ReplicaSuite, StoreTest3Replica)

   BOOST_AUTO_TEST_CASE(Add_SingleTransaction_ShouldSucceed)
   {
      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, value));
   }

   BOOST_AUTO_TEST_CASE(AddUpdate_SingleTransaction_ShouldSucceed)
   {
      int key = 5;
      int value = 6;
      int updatedValue = 7;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, updatedValue));
         SyncAwait(tx->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, updatedValue));
   }

   BOOST_AUTO_TEST_CASE(AddDelete_SingleTransaction_ShouldSucceed)
   {
      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx->CommitAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(AddDeleteAdd_SingleTransaction_ShouldSucceed)
   {
      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, value));
   }

   BOOST_AUTO_TEST_CASE(NoAdd_Delete_ShouldFail)
   {
      int key = 5;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx->AbortAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(AddDeleteUpdate_SingleTransaction_ShouldFail)
   {
      int key = 5;
      int value = 6;
      int updateValue = 7;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         bool result_remove = SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         VERIFY_ARE_EQUAL(result_remove, true);
         SyncAwait(VerifyKeyDoesNotExistAsync(*Store, *tx->StoreTransactionSPtr, key));
         bool result_update = SyncAwait(Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
         VERIFY_ARE_EQUAL(result_update, false);
         SyncAwait(tx->AbortAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(AddAdd_SingleTransaction_ShouldFail)
   {
      bool secondAddFailed = false;

      int key = 5;
      int value = 6;

      WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
      SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

      try
      {
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
      }
      catch (ktl::Exception &)
      {
         // todo once status codes are fixed, fix this as well
         secondAddFailed = true;
      }

      CODING_ERROR_ASSERT(secondAddFailed == true);
      SyncAwait(tx->AbortAsync());

      KBufferSerializer::SPtr serializer1 = nullptr;
      KBufferSerializer::Create(GetAllocator(), serializer1);

      StringStateSerializer::SPtr serializer2 = nullptr;
      StringStateSerializer::Create(GetAllocator(), serializer2);
   }

   BOOST_AUTO_TEST_CASE(MultipleAdds_SingleTransaction_ShouldSucceed)
   {
       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();

           for (int i = 0; i < 10; i++)
           {
               SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
           }

           SyncAwait(tx->CommitAsync());
       }

      for (int i = 0; i < 10; i++)
      {
         int expectedValue = i;
         SyncAwait(VerifyKeyExistsInStoresAsync(i, -1, expectedValue));
      }
   }

   BOOST_AUTO_TEST_CASE(MultipleUpdates_SingleTransaction_ShouldSucceed)
   {
       int key = 5;
       int value = -1;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

           for (int i = 0; i < 10; i++)
           {
               SyncAwait(Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, i, DefaultTimeout, CancellationToken::None));
           }

           SyncAwait(tx->CommitAsync());
       }

      int expectedValue = 9;
      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, expectedValue));
   }

   BOOST_AUTO_TEST_CASE(AddUpdate_DifferentTransactions_ShouldSucceed)
   {
      int key = 5;
      int value = 5;
      int updateValue = 6;

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx1->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx2->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, updateValue));
   }

   BOOST_AUTO_TEST_CASE(AddDelete_DifferentTransactions_ShouldSucceed)
   {
      int key = 5;
      int value = 5;

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx1->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         bool result = SyncAwait(Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         CODING_ERROR_ASSERT(result == true);
         SyncAwait(tx2->CommitAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(AddUpdateRead_DifferentTransactions_ShouldSucceed)
   {
      int key = 5;
      int value = 5;
      int updateValue = 7;

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx1->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         bool res = SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
         CODING_ERROR_ASSERT(res == true);
         SyncAwait(tx2->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, updateValue));
   }

   BOOST_AUTO_TEST_CASE(AddDeleteUpdate_DifferentTransactions_ShouldFail)
   {
      int key = 5;
      int value = 5;
      int updateValue = 7;

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx1->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         bool result = SyncAwait(Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
         CODING_ERROR_ASSERT(result == true);
         SyncAwait(tx2->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
         bool res = SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
         CODING_ERROR_ASSERT(res == false);
         SyncAwait(tx3->AbortAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(MultipleAdds_MultipleTransactions_ShouldSucceed)
   {
      for (int i = 0; i < 10; i++)
      {
         int key = i;
         int value = i;

         {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx->CommitAsync());
         }
      }

      {
          for (ULONG m = 0; m < Stores->Count(); m++)
          {
              WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[m]);
              for (int i = 0; i < 10; i++)
              {
                  SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx->StoreTransactionSPtr, i, -1, i));
              }

              SyncAwait(tx->AbortAsync());
          }
      }
   }

   BOOST_AUTO_TEST_CASE(MultipleAddsUpdates_MultipleTransactions_ShouldSucceed)
   {
      for (int i = 0; i < 10; i++)
      {
         int key = i;
         int value = i;

         {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx->CommitAsync());
         }
      }

      for (int i = 0; i < 10; i++)
      {
         int key = i;
         int value = i + 10;

         {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx->CommitAsync());
         }
      }

      {
          for (ULONG m = 0; m < Stores->Count(); m++)
          {
              WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[m]);
              for (int i = 0; i < 10; i++)
              {
                  SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx->StoreTransactionSPtr, i, -1, i + 10));
              }

              SyncAwait(tx->AbortAsync());
          }
      }
   }

   BOOST_AUTO_TEST_CASE(Add_Abort_ShouldSucceed)
   {
      int key = 5;
      int value = 5;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         KeyValuePair<LONG64, int> kvpair(-1, -1);
         SyncAwait(Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, 5, DefaultTimeout, kvpair, CancellationToken::None));
         CODING_ERROR_ASSERT(kvpair.Value == value);
         SyncAwait(tx->AbortAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(AddAdd_SameKeyOnConcurrentTransaction_ShouldTimeout)
   {
      bool hasFailed = false;

      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         try
         {
            SyncAwait(Store->AddAsync(*tx2->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         }
         catch (ktl::Exception & e)
         {
            hasFailed = true;
            CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
         }

         CODING_ERROR_ASSERT(hasFailed);

         SyncAwait(tx2->AbortAsync());
         SyncAwait(tx1->AbortAsync());
      }

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(UpdateRead_SameKeyOnConcurrentTransaction_ShouldTimeout)
   {
      bool hasFailed = false;

      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx->CommitAsync());
      }

      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, key, 8, DefaultTimeout, CancellationToken::None));

         {
            WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
            tx2->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
            try
            {
                SyncAwait(VerifyKeyExistsAsync(*Store, *tx2->StoreTransactionSPtr, key, -1, 8));
            }
            catch (ktl::Exception & e)
            {
               hasFailed = true;
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
            }

            CODING_ERROR_ASSERT(hasFailed);

            SyncAwait(tx2->AbortAsync());
            SyncAwait(tx1->AbortAsync());
         }
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, value));
   }

   BOOST_AUTO_TEST_CASE(WriteStore_NotPrimary_ShouldFail)
   {
      Replicator->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
      bool hasFailed = false;

      int key = 5;
      int value = 6;

      WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
      try
      {
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
      }
      catch (ktl::Exception & e)
      {
         hasFailed = true;
         CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_PRIMARY);
      }

      CODING_ERROR_ASSERT(hasFailed);
      SyncAwait(tx->AbortAsync());

      SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
   }

   BOOST_AUTO_TEST_CASE(ReadStore_NotReadable_ShouldFail)
   {
      bool hasFailed = false;

      int key = 5;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx->CommitAsync());
      }

      Replicator->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

      WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
      try
      {
          SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
      }
      catch (ktl::Exception & e)
      {
         hasFailed = true;
         CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_READABLE);
      }
      SyncAwait(tx->AbortAsync());
      CODING_ERROR_ASSERT(hasFailed);
   }

   BOOST_AUTO_TEST_CASE(ReadStore_ActiveSecondary_NotReadable_ShouldFail)
   {
       bool hasFailed = false;

       int key = 5;
       int value = 6;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
           SyncAwait(tx->CommitAsync());
       }
        
       Replicator->Role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
       Replicator->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
       Replicator->SetReadable(false);

       WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
       try
       {
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
       }
       catch (ktl::Exception & e)
       {
           hasFailed = true;
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_READABLE);
       }
       SyncAwait(tx->AbortAsync());
       CODING_ERROR_ASSERT(hasFailed);
   }

   BOOST_AUTO_TEST_CASE(ReadStore_ActiveSecondary_IsReadable_NotSnapshotTxn_ShouldFail)
   {
       bool hasFailed = false;

       int key = 5;
       int value = 6;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
           SyncAwait(tx->CommitAsync());
       }
        
       Replicator->Role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
       Replicator->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
       Replicator->SetReadable(true);

       WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
       tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
       try
       {
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
       }
       catch (ktl::Exception & e)
       {
           hasFailed = true;
           CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_READABLE);
       }
       SyncAwait(tx->AbortAsync());
       CODING_ERROR_ASSERT(hasFailed);
   }

   BOOST_AUTO_TEST_CASE(ReadStore_ActiveSecondary_IsReadable_SnapshotTxn_ShouldSucceed)
   {
       int key = 5;
       int value = 6;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
           SyncAwait(tx->CommitAsync());
       }
        
       Replicator->Role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
       Replicator->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
       Replicator->SetReadable(true);

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
           SyncAwait(tx->AbortAsync());
       }
   }

   BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnapshotContainer_MovedFromDifferentialState)
   {
      int key = 5;
      int value = 6;

      // Add
      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx1->CommitAsync());
      }

      // Start the snapshot transaction here
      KArray<WriteTransaction<int, int>::SPtr> storesTransactions(GetAllocator());

      for (ULONG i = 0; i < Stores->Count(); i++)
      {
          WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
          tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
          storesTransactions.Append(tx);
          SyncAwait(VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, -1, value));
      }

      // Update causes entries to previous version.
      {
         WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
         SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, 7, DefaultTimeout, CancellationToken::None));
         SyncAwait(tx2->CommitAsync());
      }

      // Update again to move entries to snapshot container
      {
          WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
          SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, 8, DefaultTimeout, CancellationToken::None));
          SyncAwait(tx2->CommitAsync());
      }

      SyncAwait(VerifyKeyExistsInStoresAsync(key, -1, 8));

      for (ULONG i = 0; i < storesTransactions.Count(); i++)
      {
          SyncAwait(VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, -1, value));
          SyncAwait(storesTransactions[i]->AbortAsync());
      }

      storesTransactions.Clear();
   }

   BOOST_AUTO_TEST_CASE(SnapshotRead_FromConsolidatedState)
   {
      ULONG32 count = 4;

      for (ULONG32 idx = 0; idx < count; idx++)
      {
         {
            WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
         }
      }

      // Start the snapshot transaction here
      KArray<WriteTransaction<int, int>::SPtr> storesTransactions(GetAllocator());

      for (ULONG i = 0; i < Stores->Count(); i++)
      {
          WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
          tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
          storesTransactions.Append(tx);
          for (ULONG32 idx = 0; idx < count; idx++)
          {
              SyncAwait(VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, idx, -1, idx));
          }
      }

      Checkpoint();

      // Update after checkpoint.
      for (ULONG32 idx = 0; idx < count; idx++)
      {
         {
            WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx+10, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
         }
      }

      for (ULONG32 idx = 0; idx < count; idx++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(idx, -1, idx+10));
      }

      for (ULONG i = 0; i < storesTransactions.Count(); i++)
      {
          for (ULONG32 idx = 0; idx < count; idx++)
          {
              SyncAwait(VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, idx, -1, idx));
          }

          SyncAwait(storesTransactions[i]->AbortAsync());
      }

      storesTransactions.Clear();
   }

   BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnapshotContainer_MovedFromDifferential_DuringConsolidation)
   {
      ULONG32 count = 4;
      for (ULONG32 idx = 0; idx < count; idx++)
      {
         {
            WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
         }
      }

      // Start the snapshot transaction here
      KArray<WriteTransaction<int, int>::SPtr> storesTransactions(GetAllocator());

      for (ULONG i = 0; i < Stores->Count(); i++)
      {
          WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
          tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
          storesTransactions.Append(tx);
          for (ULONG32 idx = 0; idx < count; idx++)
          {
              SyncAwait(VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, idx, -1, idx));
          }
      }

      for (ULONG32 idx = 0; idx < count; idx++)
      {
         {
            WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 10, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
         }
      }

      // Read updated value to validate.
      for (ULONG32 idx = 0; idx < count; idx++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(idx, -1, idx + 10));
      }

      Checkpoint();

      for (ULONG32 idx = 0; idx < count; idx++)
      {
         {
            WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 20, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
         }
      }

      // Read updated value to validate.
      for (ULONG32 idx = 0; idx < count; idx++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(idx, -1, idx + 20));
      }

      for (ULONG i = 0; i < storesTransactions.Count(); i++)
      {
          for (ULONG32 idx = 0; idx < count; idx++)
          {
              SyncAwait(VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, idx, -1, idx));
          }

          SyncAwait(storesTransactions[i]->AbortAsync());
      }

      storesTransactions.Clear();
   }

   BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnapshotContainer_MovedFromConsolidatedState)
   {
       ULONG32 count = 4;
       for (ULONG32 idx = 0; idx < count; idx++)
       {
           {
               WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
               SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None));
               SyncAwait(tx1->CommitAsync());
           }
       }

       // Start the snapshot transaction here
       KArray<WriteTransaction<int, int>::SPtr> storesTransactions(GetAllocator());

       for (ULONG i = 0; i < Stores->Count(); i++)
       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
           tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
           storesTransactions.Append(tx);
           for (ULONG32 idx = 0; idx < count; idx++)
           {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, idx, -1, idx));
           }
       }

       Checkpoint();

       for (ULONG32 idx = 0; idx < count; idx++)
       {
           {
               WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
               SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 10, DefaultTimeout, CancellationToken::None));
               SyncAwait(tx1->CommitAsync());
           }
       }

       Checkpoint();

       // Read updated value to validate.
       for (ULONG32 idx = 0; idx < count; idx++)
       {
           SyncAwait(VerifyKeyExistsInStoresAsync(idx, -1, idx + 10));
       }

       for (ULONG i = 0; i < storesTransactions.Count(); i++)
       {
           for (ULONG32 idx = 0; idx < count; idx++)
           {
               SyncAwait(VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, idx, -1, idx));
           }

           SyncAwait(storesTransactions[i]->AbortAsync());
       }

       storesTransactions.Clear();
   }

   BOOST_AUTO_TEST_SUITE_END()
}
