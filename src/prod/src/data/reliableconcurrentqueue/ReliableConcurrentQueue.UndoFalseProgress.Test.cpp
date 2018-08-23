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

    class UndoFalseProgressTest : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        UndoFalseProgressTest()
        {
            NTSTATUS status = TestStateSerializer<LONG64>::Create(GetAllocator(), long64SerializerSPtr_);
            Diagnostics::Validate(status);
            status = TestStateSerializer<int>::Create(GetAllocator(), intSerializerSPtr_);
            Diagnostics::Validate(status);
        }

        ~UndoFalseProgressTest()
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

        void SecondaryUndoFalseProgress(
            __in LONG64 sequenceNumber,
            __in StoreModificationType::Enum operationType, 
            __in LONG64 key)
        {
            KAllocator& allocator = GetAllocator();

            Transaction::SPtr tx = CreateReplicatorTransaction();
            Transaction::CSPtr txnCSPtr = tx.RawPtr();
            tx->CommitSequenceNumber = sequenceNumber;

            OperationData::SPtr keyBytes = GetBytes(key);

            OperationData::CSPtr metadataCSPtr = nullptr;

            KSharedPtr<MetadataOperationDataK<LONG64> const> metadataKCSPtr = nullptr;
            NTSTATUS status = MetadataOperationDataK<LONG64>::Create(
                key,
                Constants::SerializedVersion,
                operationType,
                txnCSPtr->TransactionId,
                keyBytes,
                allocator,
                metadataKCSPtr);
            Diagnostics::Validate(status);
            metadataCSPtr = static_cast<const OperationData* const>(metadataKCSPtr.RawPtr());

            auto operationContext = SyncAwait(RCQ->ApplyAsync(
                sequenceNumber,
                *txnCSPtr,
                ApplyContext::SecondaryFalseProgress,
                metadataCSPtr.RawPtr(),
                nullptr));

            if (operationContext != nullptr)
            {
                RCQ->Unlock(*operationContext);
            }
        }

        void UndoFalseProgressAdd(__in LONG64 sequenceNumber, __in LONG64 key)
        {
            SecondaryUndoFalseProgress(sequenceNumber, StoreModificationType::Enum::Add, key);
        }

        void UndoFalseProgressUpdate(__in LONG64 sequenceNumber, __in LONG64 key)
        {
            SecondaryUndoFalseProgress(sequenceNumber, StoreModificationType::Enum::Update, key);
        }

        void UndoFalseProgressRemove(__in LONG64 sequenceNumber, __in LONG64 key)
        {
            SecondaryUndoFalseProgress(sequenceNumber, StoreModificationType::Enum::Remove, key);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    private:
        KtlSystem* ktlSystem_;
        KSharedPtr<TestStateSerializer<LONG64>> long64SerializerSPtr_;
        KSharedPtr<TestStateSerializer<int>> intSerializerSPtr_;
    };

    BOOST_FIXTURE_TEST_SUITE(UndoFalseProgressTestSuite, UndoFalseProgressTest)

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_Single_Enqueue_ShouldSucceed)
    {
        LONG64 undoSequenceNumber = -1;

        {
            auto txn = CreateReplicatorTransaction();
            KFinally([&] { txn->Dispose(); });

            SyncAwait(RCQ->EnqueueAsync(*txn, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            undoSequenceNumber = txn->CommitSequenceNumber;
        }

        UndoFalseProgressAdd(undoSequenceNumber, 1);

        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_Multiple_Enqueue_ShouldSucceed)
    {
        LONG64 undoSequenceNumber = -1;

        SyncEnqueueItem(10);
        SyncEnqueueItem(20);
        SyncEnqueueItem(30);

        {
            auto txn = CreateReplicatorTransaction();
            KFinally([&] { txn->Dispose(); });

            SyncAwait(RCQ->EnqueueAsync(*txn, 40, Common::TimeSpan::MaxValue, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            undoSequenceNumber = txn->CommitSequenceNumber;
        }

        UndoFalseProgressAdd(undoSequenceNumber, 4);

        SyncExpectDequeue(10);
        SyncExpectDequeue(20);
        SyncExpectDequeue(30);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_Single_Dequeue_ShouldSucceed)
    {
        LONG64 undoSequenceNumber = -1;

        SyncEnqueueItem(10);

        {
            auto txn = CreateReplicatorTransaction();
            KFinally([&] { txn->Dispose(); });

            int value;
            bool succeeded = SyncAwait(RCQ->TryDequeueAsync(*txn, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);;

            SyncAwait(txn->CommitAsync());

            undoSequenceNumber = txn->CommitSequenceNumber;
        }

        UndoFalseProgressRemove(undoSequenceNumber, 1);

        SyncExpectDequeue(10);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_Multiple_Dequeue_ShouldSucceed)
    {
        LONG64 undoSequenceNumber = -1;

        SyncEnqueueItem(10);
        SyncEnqueueItem(20);
        SyncEnqueueItem(30); 

        {
            auto txn = CreateReplicatorTransaction();
            KFinally([&] { txn->Dispose(); });

            int value;
            bool succeeded = SyncAwait(RCQ->TryDequeueAsync(*txn, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            SyncAwait(txn->CommitAsync());

            undoSequenceNumber = txn->CommitSequenceNumber;
        }

        UndoFalseProgressRemove(undoSequenceNumber, 1);

        SyncExpectDequeue(20);
        SyncExpectDequeue(30);
        SyncExpectDequeue(10);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_Enqueue_Dequeue_ShouldSucceed)
    {
        LONG64 undoSequenceNumber = -1;

        SyncEnqueueItem(10);
        SyncEnqueueItem(20);
        SyncEnqueueItem(30); 

        SyncExpectDequeue(10);
        SyncEnqueueItem(40);

        {
            auto txn = CreateReplicatorTransaction();
            KFinally([&] { txn->Dispose(); });

            SyncAwait(RCQ->EnqueueAsync(*txn, 50, Common::TimeSpan::MaxValue, CancellationToken::None));

            int value;
            bool succeeded = SyncAwait(RCQ->TryDequeueAsync(*txn, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            SyncAwait(txn->CommitAsync());

            undoSequenceNumber = txn->CommitSequenceNumber;
        }

        UndoFalseProgressRemove(undoSequenceNumber, 2);
        UndoFalseProgressAdd(undoSequenceNumber, 5);

        SyncExpectDequeue(30);
        SyncExpectDequeue(40);
        SyncExpectDequeue(20);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_SUITE_END()
}