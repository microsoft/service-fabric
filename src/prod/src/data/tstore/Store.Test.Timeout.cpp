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

   class StoreTestTimeOut : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
   {
   public:
       StoreTestTimeOut()
      {
         Setup(3);
         Replicator->DelayAddOperationAndThrow = true;
      }

      ~StoreTestTimeOut()
      {
         Cleanup();
      }


      Common::CommonConfig config; // load the config object as its needed for the tracing to work
   };

   BOOST_FIXTURE_TEST_SUITE(StoreTestTimeOutSuite, StoreTestTimeOut)

   BOOST_AUTO_TEST_CASE(SlowAdd_ShouldTimeout)
   {
      int key1 = 5;
      int key2 = 6;
      int value = 6;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None));

         try
         {
             SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None));
             CODING_ERROR_ASSERT(false);
         }
         catch (ktl::Exception& e)
         {
             SyncAwait(tx->AbortAsync());
             CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
         }
      }
   }

   BOOST_AUTO_TEST_CASE(SlowConditionalUpdateAsync_ShouldTimeout)
   {
       int key = 5;
       int value = 6;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

           try
           {
               SyncAwait(Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
               CODING_ERROR_ASSERT(false);
           }
           catch (ktl::Exception& e)
           {
               SyncAwait(tx->AbortAsync());
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
           }
       }
   }

   BOOST_AUTO_TEST_CASE(SlowConditionalRemoveAsync_ShouldTimeout)
   {
       int key = 5;
       int value = 6;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

           try
           {
               SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
               CODING_ERROR_ASSERT(false);
           }
           catch (ktl::Exception& e)
           {
               SyncAwait(tx->AbortAsync());
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
           }
       }
   }

   BOOST_AUTO_TEST_SUITE_END()
}
