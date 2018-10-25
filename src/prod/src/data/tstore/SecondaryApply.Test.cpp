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

    class SecondaryApplyTest : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
    {
    public:
        SecondaryApplyTest()
        {
            Setup();
            NTSTATUS status = TestStateSerializer<int>::Create(GetAllocator(), serializerSPtr_);
            Diagnostics::Validate(status);
        }

        ~SecondaryApplyTest()
        {
            serializerSPtr_ = nullptr;
            Cleanup();
        }

        OperationData::SPtr GetBytes(__in int& value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            serializerSPtr_->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        ktl::Awaitable<void> SecondaryApplyAsync(
            __in StoreModificationType::Enum operationType, 
            __in int key, 
            __in OperationData & keyBytes, 
            __in int value, 
            __in OperationData::SPtr & valueBytes)
        {
            KAllocator& allocator = GetAllocator();
            auto commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            Transaction::SPtr tx = CreateReplicatorTransaction();
            Transaction::CSPtr txnCSPtr = tx.RawPtr();
            tx->CommitSequenceNumber = commitLSN;


            OperationData::CSPtr metadataCSPtr = nullptr;

            if (valueBytes != nullptr)
            {
                KSharedPtr<MetadataOperationDataKV<int, int> const> metadataKVCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataKV<int, int>::Create(
                    key,
                    value,
                    Constants::SerializedVersion,
                    operationType,
                    txnCSPtr->TransactionId,
                    &keyBytes,
                    allocator,
                    metadataKVCSPtr);
                Diagnostics::Validate(status);
                metadataCSPtr = static_cast<const OperationData* const>(metadataKVCSPtr.RawPtr());
            }
            else
            {
                KSharedPtr<MetadataOperationDataK<int> const> metadataKCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataK<int>::Create(
                    key,
                    Constants::SerializedVersion,
                    operationType,
                    txnCSPtr->TransactionId,
                    &keyBytes,
                    allocator,
                    metadataKCSPtr);
                Diagnostics::Validate(status);
                metadataCSPtr = static_cast<const OperationData* const>(metadataKCSPtr.RawPtr());
            }

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          RedoUndoOperationData::Create(GetAllocator(), valueBytes, nullptr, redoDataSPtr);

          auto operationContext = co_await Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::SecondaryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr());

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }

          co_return;
        }

        Awaitable<void> SecondaryAddAsync(int key, int value)
        {
            auto keyBytesSPtr = GetBytes(key);
            auto valueBytesSPtr = GetBytes(value);
            co_await SecondaryApplyAsync(StoreModificationType::Enum::Add, key, *keyBytesSPtr, value, valueBytesSPtr);
            co_return;
        }

        Awaitable<void> SecondaryUpdateAsync(int key, int value)
        {
            auto bytesSPtr = GetBytes(key);
            auto valueBytesSPtr = GetBytes(value);
            co_await SecondaryApplyAsync(StoreModificationType::Enum::Update, key, *bytesSPtr, value, valueBytesSPtr);
            co_return;
        }

        Awaitable<void> SecondaryRemoveAsync(int key)
        {
            OperationData::SPtr valueBytesSPtr = nullptr;
            auto bytesSPtr = GetBytes(key);
            co_await SecondaryApplyAsync(StoreModificationType::Enum::Remove, key, *bytesSPtr, -1, valueBytesSPtr);
            co_return;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
        KSharedPtr<TestStateSerializer<int>> serializerSPtr_;

#pragma region test functions
    public:
        ktl::Awaitable<void> Secondary_AddOperation_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;
            co_await SecondaryAddAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
            co_return;
        }

        ktl::Awaitable<void> Secondary_UpdateOperation_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;
            int updatedValue = 9;

            co_await SecondaryAddAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await SecondaryUpdateAsync(key, updatedValue);
            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);
            co_return;
        }

        ktl::Awaitable<void> Secondary_RemoveOperation_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;

            co_await SecondaryAddAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await SecondaryRemoveAsync(key);
            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(SecondaryApplyTestSuite, SecondaryApplyTest)

    BOOST_AUTO_TEST_CASE(Secondary_AddOperation_ShouldSucceed)
    {
        SyncAwait(Secondary_AddOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Secondary_UpdateOperation_ShouldSucceed)
    {
        SyncAwait(Secondary_UpdateOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Secondary_RemoveOperation_ShouldSucceed)
    {
        SyncAwait(Secondary_RemoveOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
