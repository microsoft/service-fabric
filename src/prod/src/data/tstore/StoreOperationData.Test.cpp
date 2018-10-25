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
   using namespace Data::Utilities;

   class StoreOperationDataTest
   {
   public:
      StoreOperationDataTest()
      {
         NTSTATUS status;
         status = KtlSystem::Initialize(FALSE, &ktlSystem_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         ktlSystem_->SetStrictAllocationChecks(TRUE);
      }

      ~StoreOperationDataTest()
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
        ktl::Awaitable<void> Metadata_NullKeyBytes_ShouldSucceed_Test()
       {
          KAllocator& allocator = GetAllocator();

          ULONG32 version = 1;
          StoreModificationType::Enum modificationType = StoreModificationType::Enum::Add;
          LONG64 transactionId = 5;

          MetadataOperationData::CSPtr metadataOperationDataSPtr;
          MetadataOperationData::Create(version, modificationType, transactionId, nullptr, allocator, metadataOperationDataSPtr);

          OperationData::CSPtr operationdataSPtr(metadataOperationDataSPtr.RawPtr());
          MetadataOperationData::CSPtr deserializedData = MetadataOperationData::Deserialize(1, allocator, operationdataSPtr);

          CODING_ERROR_ASSERT(deserializedData->KeyBytes == nullptr);
          CODING_ERROR_ASSERT(deserializedData->ModificationType == modificationType);
          CODING_ERROR_ASSERT(deserializedData->TransactionId == transactionId);
           co_return;
       }

        ktl::Awaitable<void> Metadata_NonNullKeyBytes_ShouldSucceed_Test()
       {
          KAllocator& allocator = GetAllocator();

          ULONG32 version = 1;
          StoreModificationType::Enum modificationType = StoreModificationType::Enum::Add;
          LONG64 transactionId = 5;

          // Create key bytes
          BinaryWriter binaryWriter(allocator);
          binaryWriter.Write(5);
          OperationData::SPtr operationDataSPtr = OperationData::Create(allocator);
          operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

          MetadataOperationData::CSPtr metadataOperationDataSPtr;
          MetadataOperationData::Create(version, modificationType, transactionId, operationDataSPtr, allocator, metadataOperationDataSPtr);

          OperationData::CSPtr operationdataSPtr(metadataOperationDataSPtr.RawPtr());
          MetadataOperationData::CSPtr deserializedData = MetadataOperationData::Deserialize(1, allocator, operationdataSPtr);

          CODING_ERROR_ASSERT(deserializedData->KeyBytes->BufferCount == operationDataSPtr->BufferCount);
          CODING_ERROR_ASSERT(deserializedData->ModificationType == modificationType);
          CODING_ERROR_ASSERT(deserializedData->TransactionId == transactionId);
           co_return;
       }

        ktl::Awaitable<void> RedoUndo_NullValue_ShouldSucceed_Test()
       {
          KAllocator& allocator = GetAllocator();

          RedoUndoOperationData::SPtr redoUndoSPtr;
          RedoUndoOperationData::Create(allocator, nullptr, nullptr, redoUndoSPtr);

          OperationData::SPtr operationdataSPtr = redoUndoSPtr.DownCast<OperationData>();
          RedoUndoOperationData::SPtr deserializedData = RedoUndoOperationData::Deserialize(*operationdataSPtr, allocator);

          CODING_ERROR_ASSERT(deserializedData->ValueOperationData == nullptr);
          CODING_ERROR_ASSERT(deserializedData->NewValueOperationData == nullptr);
           co_return;
       }

        ktl::Awaitable<void> RedoUndo_NonNullValue_ShouldSucceed_Test()
       {
          KAllocator& allocator = GetAllocator();

          // Create key bytes
          BinaryWriter binaryWriter(allocator);
          binaryWriter.Write(5);
          OperationData::SPtr operationDataSPtr = OperationData::Create(allocator);
          operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

          RedoUndoOperationData::SPtr redoUndoSPtr;
          RedoUndoOperationData::Create(allocator, operationDataSPtr, nullptr, redoUndoSPtr);

          OperationData::SPtr operationdataSPtr = redoUndoSPtr.DownCast<OperationData>();
          RedoUndoOperationData::SPtr deserializedData = RedoUndoOperationData::Deserialize(*operationdataSPtr, allocator);

          CODING_ERROR_ASSERT(deserializedData->ValueOperationData->BufferCount == operationDataSPtr->BufferCount);
          CODING_ERROR_ASSERT(deserializedData->NewValueOperationData == nullptr);
           co_return;
       }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(StoreOperationDataTestSuite, StoreOperationDataTest)

   BOOST_AUTO_TEST_CASE(Metadata_NullKeyBytes_ShouldSucceed)
   {
       SyncAwait(Metadata_NullKeyBytes_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(Metadata_NonNullKeyBytes_ShouldSucceed)
   {
       SyncAwait(Metadata_NonNullKeyBytes_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(RedoUndo_NullValue_ShouldSucceed)
   {
       SyncAwait(RedoUndo_NullValue_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(RedoUndo_NonNullValue_ShouldSucceed)
   {
       SyncAwait(RedoUndo_NonNullValue_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_SUITE_END()
}
