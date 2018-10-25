// ----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReliableConcurrentQueue.h"
#include "ReliableConcurrentQueue.TestBase.h"
#include "../tstore/TStoreTestBase.h"

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data::TStore;
    using namespace Data::Utilities;

    class SecondaryApplyTest : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        SecondaryApplyTest()
        {
            NTSTATUS status = TestStateSerializer<LONG64>::Create(GetAllocator(), long64SerializerSPtr_);
            Diagnostics::Validate(status);
            status = TestStateSerializer<int>::Create(GetAllocator(), intSerializerSPtr_);
            Diagnostics::Validate(status);
        }

        ~SecondaryApplyTest()
        {
            long64SerializerSPtr_ = nullptr;
            intSerializerSPtr_ = nullptr;
        }

        OperationData::SPtr GetBytes(__in LONG64 value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            long64SerializerSPtr_->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        OperationData::SPtr GetBytes(__in int value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            intSerializerSPtr_->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        void SecondaryApply(
            __in StoreModificationType::Enum operationType, 
            __in LONG64 key, 
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
                KSharedPtr<MetadataOperationDataKV<LONG64, int> const> metadataKVCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataKV<LONG64, int>::Create(
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
                KSharedPtr<MetadataOperationDataK<LONG64> const> metadataKCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataK<LONG64>::Create(
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

            auto operationContext = SyncAwait(RCQ->ApplyAsync(
                commitLSN,
                *txnCSPtr,
                ApplyContext::SecondaryRedo,
                metadataCSPtr.RawPtr(),
                redoDataSPtr.RawPtr()));

            if (operationContext != nullptr)
            {
                RCQ->Unlock(*operationContext);
            }
        }

        void SecondaryAdd(LONG64 key, int value)
        {
            auto keyBytesSPtr = GetBytes(key);
            auto valueBytesSPtr = GetBytes(value);
            SecondaryApply(StoreModificationType::Enum::Add, key, *keyBytesSPtr, value, valueBytesSPtr);
        }

        void SecondaryRemove(LONG64 key)
        {
            OperationData::SPtr valueBytesSPtr = nullptr;
            auto bytesSPtr = GetBytes(key);
            SecondaryApply(StoreModificationType::Enum::Remove, key, *bytesSPtr, -1, valueBytesSPtr);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
        KSharedPtr<TestStateSerializer<LONG64>> long64SerializerSPtr_;
        KSharedPtr<TestStateSerializer<int>> intSerializerSPtr_;
    };

    BOOST_FIXTURE_TEST_SUITE(SecondaryApplyTestSuite, SecondaryApplyTest)

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);

        SyncExpectDequeue(10);
        SyncExpectDequeue(20);
        SyncExpectDequeue(30);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Ordered_Empty_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryRemove(1);
        SecondaryRemove(2);
        SecondaryRemove(3);

        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_ReverseOrdered_Empty_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryRemove(3);
        SecondaryRemove(2);
        SecondaryRemove(1);

        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Ordered_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryAdd(4, 40);
        SecondaryRemove(1);
        SecondaryRemove(2);
        SecondaryRemove(3);

        SyncExpectDequeue(40);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_ReverseOrdered_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryAdd(4, 40);
        SecondaryRemove(3);
        SecondaryRemove(2);
        SecondaryRemove(1);

        SyncExpectDequeue(40);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Mixed_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryRemove(3);
        SecondaryAdd(4, 40);
        SecondaryRemove(2);
        SecondaryAdd(5, 50);
        SecondaryRemove(5);

        SyncExpectDequeue(10);
        SyncExpectDequeue(40);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Mixed2_ShouldSucceed)
    {
        SecondaryAdd(1, 10);
        SecondaryAdd(2, 20);
        SecondaryAdd(3, 30);
        SecondaryRemove(3);
        SecondaryAdd(4, 40);
        SecondaryRemove(2);
        SecondaryAdd(5, 50);
        SecondaryRemove(5);
        SecondaryRemove(1);

        SyncExpectDequeue(40);
        SyncExpectEmpty();;
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Cross_Segment_Empty_ShouldSucceed)
    {
        int iter = QUEUE_SEGMENT_MAX_SIZE * 2;
        for (int i = 1; i <= iter; ++i)
        {
            SecondaryAdd(i, i * 10);
        }

        for (int i = iter; i >= 1; --i)
        {
            SecondaryRemove(i);
        }

        SyncExpectEmpty();;
    }

    BOOST_AUTO_TEST_CASE(Secondary_Enqueue_Dequeue_Cross_Segment_NotEmpty_ShouldSucceed)
    {
        int iter = QUEUE_SEGMENT_MAX_SIZE * 2;
        for (int i = 1; i <= iter; ++i)
        {
            SecondaryAdd(i, i * 10);
        }

        for (int i = iter; i >= 1; --i)
        {
            if (i % 2 == 0)
                SecondaryRemove(i);
        }

        for (int i = 1; i <= iter; i += 2)
        {
            SyncExpectDequeue(i * 10);
        }

        SyncExpectEmpty();
    }
    BOOST_AUTO_TEST_SUITE_END()
}