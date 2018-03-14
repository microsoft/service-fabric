// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

namespace TStoreTests
{
   using namespace ktl;

   class ConsolidationTest : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
   {
   public:
      ConsolidationTest()
      {
         Setup(3);
      }

      ~ConsolidationTest()
      {
         Cleanup();
      }

      //CommonConfig config; // load the config object as its needed for the tracing to work
   };

   BOOST_FIXTURE_TEST_SUITE(ConsolidationTestSuite, ConsolidationTest)

      BOOST_AUTO_TEST_CASE(Add_ConsolidateRead_ShouldSucceed)
   {
      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx1->CommitAsync());
      }

      // Checkpoint
      Checkpoint();

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, i, -1, i));
            }

            SyncAwait(tx2->AbortAsync());
         }
      }

      CloseAndReOpenStore();

      for (int i = 0; i < 10; i++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(i, -1, i));
      }
   }

   BOOST_AUTO_TEST_CASE(AddConsolidateRead_UpdateRead_ShouldSucceed)
   {
      // 1. Add.
      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();

         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx1->CommitAsync());
      }

      // 2. Checkpoint.
      Checkpoint();

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, i, -1, i));
            }

            SyncAwait(tx2->AbortAsync());
         }
      }

      // 3. Update.
      {
         WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, i, i + 10, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx3->CommitAsync());
      }

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx4 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx4->StoreTransactionSPtr, i, -1, i + 10));
            }

            SyncAwait(tx4->AbortAsync());
         }
      }

      // 4. Checkpoint again.

      Checkpoint();

      // 5. Reopen and verify.
      CloseAndReOpenStore();

      for (int i = 0; i < 10; i++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(i, -1, i + 10));
      }
   }

   BOOST_AUTO_TEST_CASE(AddConsolidateRead_UpdateConsolidateRead_ShouldSucceed)
   {
      // 1. Add.
      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();

         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx1->CommitAsync());
      }

      // 2. Checkpoint.
      Checkpoint();

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, i, -1, i));
            }

            SyncAwait(tx2->AbortAsync());
         }
      }

      // 3. Update.
      {
         WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, i, i + 10, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx3->CommitAsync());
      }

      // 5. Checkpoint.
      Checkpoint();

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx4 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx4->StoreTransactionSPtr, i, -1, i + 10));
            }

            SyncAwait(tx4->AbortAsync());
         }
      }

      // 6. Reopen and verify.
      CloseAndReOpenStore();

      for (int i = 0; i < 10; i++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(i, -1, i + 10));
      }
   }

   BOOST_AUTO_TEST_CASE(AddCheckpoint_ReOpenUpdate_ShouldSucceed)
   {
      // 1. Add.
      {
         WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx1->CommitAsync());
      }

      // 2. Checkpoint.
      Checkpoint();

      {
         for (ULONG m = 0; m < Stores->Count(); m++)
         {
            WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
            for (int i = 0; i < 10; i++)
            {
               SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, i, -1, i));
            }

            SyncAwait(tx2->AbortAsync());
         }
      }

      // 3. ReOpen.
      CloseAndReOpenStore();

      // 4. Update.
      {
         WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
         for (int i = 0; i < 10; i++)
         {
            SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, i, i + 10, DefaultTimeout, CancellationToken::None));
         }

         SyncAwait(tx3->CommitAsync());
      }

      for (int i = 0; i < 10; i++)
      {
         SyncAwait(VerifyKeyExistsInStoresAsync(i, -1, i + 10));
      }
   }


   BOOST_AUTO_TEST_CASE(Add_Checkpoint_Commit)
   {
      int key1 = 5;
      int key2 = 7;
      int value = 10;

      // 1. Add.
      WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
      SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None));
      SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None));

      // 2. Checkpoint.
      Checkpoint();

      // 3. Apply
      SyncAwait(tx1->CommitAsync());

      // 4. Dispose
      tx1 = nullptr;

      SyncAwait(VerifyKeyExistsInStoresAsync(key1, -1, value)); 
      SyncAwait(VerifyKeyExistsInStoresAsync(key2, -1, value));
   }

   BOOST_AUTO_TEST_SUITE_END()
}
