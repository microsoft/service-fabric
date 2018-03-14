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

        void SecondaryApply(
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

          auto operationContext = SyncAwait(Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::SecondaryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr()));

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }
        }

        void SecondaryAdd(int key, int value)
        {
	    auto keyBytesSPtr = GetBytes(key);
	    auto valueBytesSPtr = GetBytes(value);
            SecondaryApply(StoreModificationType::Enum::Add, key, *keyBytesSPtr, value, valueBytesSPtr);
        }

        void SecondaryUpdate(int key, int value)
        {
	    auto bytesSPtr = GetBytes(key);
	    auto valueBytesSPtr = GetBytes(value);
            SecondaryApply(StoreModificationType::Enum::Update, key, *bytesSPtr, value, valueBytesSPtr);
        }

        void SecondaryRemove(int key)
        {
            OperationData::SPtr valueBytesSPtr = nullptr;
	    auto bytesSPtr = GetBytes(key);
            SecondaryApply(StoreModificationType::Enum::Remove, key, *bytesSPtr, -1, valueBytesSPtr);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
        KSharedPtr<TestStateSerializer<int>> serializerSPtr_;
    };

    BOOST_FIXTURE_TEST_SUITE(SecondaryApplyTestSuite, SecondaryApplyTest)

    BOOST_AUTO_TEST_CASE(Secondary_AddOperation_ShouldSucceed)
    {
        int key = 7;
        int value = 6;
        SecondaryAdd(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));
    }

    BOOST_AUTO_TEST_CASE(Secondary_UpdateOperation_ShouldSucceed)
    {
        int key = 7;
        int value = 6;
        int updatedValue = 9;

        SecondaryAdd(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        SecondaryUpdate(key, updatedValue);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, updatedValue));
    }

    BOOST_AUTO_TEST_CASE(Secondary_RemoveOperation_ShouldSucceed)
    {
        int key = 7;
        int value = 6;

        SecondaryAdd(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        SecondaryRemove(key);
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
