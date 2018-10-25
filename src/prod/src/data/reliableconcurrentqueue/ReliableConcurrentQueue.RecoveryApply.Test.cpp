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
    using namespace Data::Utilities;
    using namespace Data::Collections;
    using namespace TxnReplicator;

    class RecoveryApply : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        Common::CommonConfig config;

    public:
        RecoveryApply()
        {
            NTSTATUS status = TestStateSerializer<LONG64>::Create(GetAllocator(), long64SerializerSPtr_);
            Diagnostics::Validate(status);
            status = TestStateSerializer<int>::Create(GetAllocator(), intSerializerSPtr_);
            Diagnostics::Validate(status);
        }

        void CheckpointAndRecover()
        {
            Checkpoint();
            CloseAndReOpenStore();
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

        void RecoverAdd(LONG64 key, int value, LONG64 commitLSN = -1)
        {
            KAllocator& allocator = GetAllocator();

            if (commitLSN == -1) commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            Transaction::SPtr tx = CreateReplicatorTransaction();
            Transaction::CSPtr txnCSPtr = tx.RawPtr();

            tx->CommitSequenceNumber = commitLSN;

            KSharedPtr<MetadataOperationDataKV<LONG64, int> const> metadataCSPtr = nullptr;
            NTSTATUS status = MetadataOperationDataKV<LONG64, int>::Create(
                key,
                value,
                Constants::SerializedVersion,
                StoreModificationType::Enum::Add,
                txnCSPtr->TransactionId,
                GetBytes(key),
                allocator,
                metadataCSPtr);
            Diagnostics::Validate(status);

            RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
            RedoUndoOperationData::Create(GetAllocator(), GetBytes(value), nullptr, redoDataSPtr);

            OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
            OperationData::SPtr undoSPtr = nullptr;

            auto operationContext = SyncAwait(RCQ->ApplyAsync(
                commitLSN,
                *txnCSPtr,
                ApplyContext::RecoveryRedo,
                metadataCSPtr.RawPtr(),
                redoDataSPtr.RawPtr()));

            if (operationContext != nullptr)
            {
                RCQ->Unlock(*operationContext);
            }
        }
        
        void RecoverRemove(LONG64 key, LONG64 commitLSN = -1)
        {
            KAllocator& allocator = GetAllocator();

            if (commitLSN == -1) commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            Transaction::SPtr tx = CreateReplicatorTransaction();
            Transaction::CSPtr txnCSPtr = tx.RawPtr();
            tx->CommitSequenceNumber = commitLSN;

            KSharedPtr<MetadataOperationDataK<LONG64> const> metadataCSPtr = nullptr;
            NTSTATUS status = MetadataOperationDataK<LONG64>::Create(
                key,
                Constants::SerializedVersion,
                StoreModificationType::Enum::Remove,
                txnCSPtr->TransactionId,
                GetBytes(key),
                allocator,
                metadataCSPtr);
            Diagnostics::Validate(status);

            RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
            RedoUndoOperationData::Create(GetAllocator(), nullptr, nullptr, redoDataSPtr);

            OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
            OperationData::SPtr undoSPtr = nullptr;

            auto operationContext = SyncAwait(RCQ->ApplyAsync(
                commitLSN,
                *txnCSPtr,
                ApplyContext::RecoveryRedo,
                metadataCSPtr.RawPtr(),
                redoDataSPtr.RawPtr()));

            if (operationContext != nullptr)
            {
                RCQ->Unlock(*operationContext);
            }
        }

    public:
        KSharedPtr<TestStateSerializer<LONG64>> long64SerializerSPtr_;
        KSharedPtr<TestStateSerializer<int>> intSerializerSPtr_;
    };

    BOOST_FIXTURE_TEST_SUITE(RecoveryApplyTestSuite, RecoveryApply)

    BOOST_AUTO_TEST_CASE(Single_Enqueue_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);

            CheckpointAndRecover();

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);
            co_await this->EnqueueItem(20);
            co_await this->EnqueueItem(30);

            CheckpointAndRecover();

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 30);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Empty_Enqueue_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {

            CheckpointAndRecover();

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Enqueue_Dequeue_Empty_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);
            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);

            CheckpointAndRecover();

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Single_Enqueue_NotInCheckpoint_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {

            co_await this->EnqueueItem(10);

            CloseAndReOpenStore();  // Recover without checkpointing again

            RecoverAdd(1, 10);      // Replay the enqueue operation manually

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_NotInCheckpoint_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);
            co_await this->EnqueueItem(20);
            co_await this->EnqueueItem(30);

            CloseAndReOpenStore();  // Recover without checkpointing again

            RecoverAdd(1, 10);      // Replay the enqueue operations manually
            RecoverAdd(2, 20);
            RecoverAdd(3, 30);

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 30);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_NotAllInCheckpoint_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);

            Checkpoint();

            co_await this->EnqueueItem(20);
            co_await this->EnqueueItem(30);

            CloseAndReOpenStore();  // Recover without checkpointing again

            RecoverAdd(2, 20);      // Replay the enqueue operation manually
            RecoverAdd(3, 30);

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 30);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Enqueue_Dequeue_NotAllInCheckpoint_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);
            co_await this->EnqueueItem(20);
            co_await this->EnqueueItem(30);

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            Checkpoint();

            co_await this->EnqueueItem(40);
            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            CloseAndReOpenStore();  // Recover without checkpointing again

            RecoverAdd(4, 40);      // Replay the enqueue operation manually
            RecoverRemove(2);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 30);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 40);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Enqueue_Dequeue_Empty_DequeueNotInCheckpoint_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);

            Checkpoint();

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);

            CloseAndReOpenStore();

            RecoverRemove(1);       // Replay the dequeue manually

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Enqueue_Dequeue_ConcurrentOrderApply_Recover)
    {
        SyncAwait([=]() -> Awaitable<void> {
            co_await this->EnqueueItem(10);
            co_await this->EnqueueItem(20);
            co_await this->EnqueueItem(30);

            int value;
            bool succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 10);

            co_await this->EnqueueItem(40);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 20);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 30);

            CloseAndReOpenStore();  // Recover without checkpointing again

            //
            // Replay the enqueue operation manually in reverse order
            // This simulates arbitary order in secondary log
            //
            RecoverAdd(3, 30);
            RecoverAdd(1, 10);
            RecoverAdd(2, 20);

            RecoverRemove(1);

            RecoverAdd(4, 40);

            RecoverRemove(3);
            RecoverRemove(2);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == 40);

            succeeded = co_await this->DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        } ());
    }

    BOOST_AUTO_TEST_CASE(Single_Enqueue_After_Recover)
    {
        SyncEnqueueItem(10);

        CheckpointAndRecover();

        SyncEnqueueItem(20);

        SyncExpectDequeue(10);
        SyncExpectDequeue(20);
        SyncExpectEmpty();

    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_After_Recover)
    {
        SyncEnqueueItem(10);
        SyncEnqueueItem(20);
        SyncEnqueueItem(30);

        CheckpointAndRecover();

        SyncEnqueueItem(40);
        SyncEnqueueItem(50);
        SyncEnqueueItem(60);

        SyncExpectDequeue(10);
        SyncExpectDequeue(20);
        SyncExpectDequeue(30);
        SyncExpectDequeue(40);
        SyncExpectDequeue(50);
        SyncExpectDequeue(60);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_After_Recover_NoCheckpoint)
    {
        SyncEnqueueItem(10);
        SyncEnqueueItem(20);
        SyncEnqueueItem(30);

        CloseAndReOpenStore();

        RecoverAdd(1, 10);
        RecoverAdd(2, 20);
        RecoverAdd(3, 30);

        SyncEnqueueItem(40);
        SyncEnqueueItem(50);
        SyncEnqueueItem(60);

        SyncExpectDequeue(10);
        SyncExpectDequeue(20);
        SyncExpectDequeue(30);
        SyncExpectDequeue(40);
        SyncExpectDequeue(50);
        SyncExpectDequeue(60);
        SyncExpectEmpty();
    }

    BOOST_AUTO_TEST_SUITE_END()
}

