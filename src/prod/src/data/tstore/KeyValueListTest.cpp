// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
   using namespace ktl;
   using namespace Data::TStore;

   class KeyValueListTest
   {
   public:
      KeyValueListTest()
      {
         NTSTATUS status;
         status = KtlSystem::Initialize(FALSE, &ktlSystem_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         ktlSystem_->SetStrictAllocationChecks(TRUE);
      }

      ~KeyValueListTest()
      {
         ktlSystem_->Shutdown();
      }

      KAllocator& GetAllocator()
      {
         return ktlSystem_->NonPagedAllocator();
      }

   private:
      KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> GetValue_ShouldSucceed_Test()
       {
          KAllocator& allocator = KeyValueListTest::GetAllocator();
          KeyValueList<int, int>::SPtr keyValueListSPtr = nullptr;
          NTSTATUS status = KeyValueList<int, int>::Create(5, allocator, keyValueListSPtr);
          CODING_ERROR_ASSERT(NT_SUCCESS(status));

          int key = 5;
          int value = 6;

          keyValueListSPtr->Add(key, value);

          int actualValue = keyValueListSPtr->GetValue(0);
          CODING_ERROR_ASSERT(actualValue == value);
           co_return;
       }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(KeyValueListTestSuite, KeyValueListTest)

      BOOST_AUTO_TEST_CASE(GetValue_ShouldSucceed)
   {
       SyncAwait(GetValue_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_SUITE_END()
}
