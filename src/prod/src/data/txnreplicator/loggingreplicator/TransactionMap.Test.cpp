// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Common;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;

    Common::StringLiteral const TraceComponent("TransactionMapTest");

    class TransactionMapTest
    {

    protected:

        void EndTest();
        void VerifyPendingTxStats(__in TransactionMap & map, __in ULONG expected);
        void VerifyCompletedTxStats(__in TransactionMap & map, __in ULONG expected);
        BeginTransactionOperationLogRecord::SPtr CreateBeginTx(__in Transaction & transaction, __in LONG64 lsn, __in bool isSingleOperationTx);
        OperationLogRecord::SPtr CreateOperation(__in Transaction & transaction, __in LONG64 lsn);
        EndTransactionLogRecord::SPtr CreateEndTx(__in Transaction & transaction, __in bool isCommitted, __in LONG64 lsn);

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        InvalidLogRecords::SPtr invalidLogRecords_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void TransactionMapTest::EndTest()
    {
        prId_.Reset();
    }

    void TransactionMapTest::VerifyPendingTxStats(__in TransactionMap & map, __in ULONG expected)
    {
        VERIFY_ARE_EQUAL(map.latestRecords_.size(), expected);
        VERIFY_ARE_EQUAL(map.lsnPendingTransactions_.size(), expected);
        VERIFY_ARE_EQUAL(map.transactionIdPendingTransactionsPair_.size(), expected);
    }

    void TransactionMapTest::VerifyCompletedTxStats(__in TransactionMap & map, __in ULONG expected)
    {
        VERIFY_ARE_EQUAL(map.completedTransactions_.Count(), expected);
        VERIFY_ARE_EQUAL(map.unstableTransactions_.Count(), expected);
    }

    BeginTransactionOperationLogRecord::SPtr TransactionMapTest::CreateBeginTx(__in Transaction & transaction, __in LONG64 lsn, __in bool isSingleOperationTx)
    {
        OperationData::SPtr operationData = OperationData::Create(underlyingSystem_->NonPagedAllocator());

        KBuffer::SPtr buffer;
        NTSTATUS status = KBuffer::Create(
            1,
            buffer,
            underlyingSystem_->NonPagedAllocator());

        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        operationData->Append(*buffer);

        BeginTransactionOperationLogRecord::SPtr beginTxLog = BeginTransactionOperationLogRecord::Create(
            transaction,
            isSingleOperationTx,
            operationData.RawPtr(),
            operationData.RawPtr(),
            operationData.RawPtr(),
            nullptr,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            *invalidLogRecords_->Inv_TransactionLogRecord,
            underlyingSystem_->NonPagedAllocator());
        
        beginTxLog->Lsn = lsn;
        beginTxLog->RecordPosition = lsn;
        beginTxLog->Test_SetPreviousPhysicalRecordOffsetToZero();
        return beginTxLog;
    }

    OperationLogRecord::SPtr TransactionMapTest::CreateOperation(__in Transaction & transaction, __in LONG64 lsn)
    {
        OperationData::SPtr operationData = OperationData::Create(underlyingSystem_->NonPagedAllocator());

        KBuffer::SPtr buffer;
        NTSTATUS status = KBuffer::Create(
            1,
            buffer,
            underlyingSystem_->NonPagedAllocator());

        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        operationData->Append(*buffer);

        OperationLogRecord::SPtr operationLog = OperationLogRecord::Create(
            transaction,
            operationData.RawPtr(),
            operationData.RawPtr(),
            operationData.RawPtr(),
            nullptr,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            *invalidLogRecords_->Inv_TransactionLogRecord,
            underlyingSystem_->NonPagedAllocator());
        
        operationLog->Lsn = lsn;
        operationLog->RecordPosition = lsn;
        operationLog->Test_SetPreviousPhysicalRecordOffsetToZero();
        return operationLog;
    }

    EndTransactionLogRecord::SPtr TransactionMapTest::CreateEndTx(__in Transaction & transaction, __in bool isCommitted, __in LONG64 lsn)
    {
        EndTransactionLogRecord::SPtr endTxLog = EndTransactionLogRecord::Create(
            transaction,
            isCommitted,
            true,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            *invalidLogRecords_->Inv_TransactionLogRecord,
            underlyingSystem_->NonPagedAllocator());
        
        endTxLog->Lsn = lsn;
        endTxLog->RecordPosition = lsn;
        endTxLog->Test_SetPreviousPhysicalRecordOffsetToZero();
        return endTxLog;
    }

    BOOST_FIXTURE_TEST_SUITE(TransactionMapTestSuite, TransactionMapTest)

    BOOST_AUTO_TEST_CASE(TxMap_VerifyChainOfTransactions_1OpTx)
    {
        TEST_TRACE_BEGIN("TxMap_VerifyChainOfTransactions_1OpTx")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
            
            BeginTransactionOperationLogRecord::SPtr createTx = CreateBeginTx(*transaction->Tx, 1, true);
            map->CreateTransaction(*createTx);
            
            VERIFY_ARE_EQUAL(createTx->ParentTransactionRecord, nullptr);
            VERIFY_ARE_EQUAL(createTx->IsEnlistedTransaction, true);

            invalidLogRecords_.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(TxMap_VerifyChainOfTransactions_3OpTx)
    {
        TEST_TRACE_BEGIN("TxMap_VerifyChainOfTransactions_3OpTx")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);

            map->CreateTransaction(*CreateBeginTx(*transaction->Tx, 1, false));
            map->AddOperation(*CreateOperation(*transaction->Tx, 2));
            map->AddOperation(*CreateOperation(*transaction->Tx, 3));

            EndTransactionLogRecord::SPtr commit = CreateEndTx(*transaction->Tx, true, 4);
            map->CompleteTransaction(*commit);
            
            TransactionLogRecord::SPtr chain = commit.RawPtr();
            VERIFY_ARE_EQUAL(chain->Lsn, 4);
            VERIFY_ARE_EQUAL(chain->ParentTransactionRecord->Lsn, 3);
            VERIFY_ARE_EQUAL(chain->ParentTransactionRecord->ParentTransactionRecord->Lsn, 2);
            VERIFY_ARE_EQUAL(chain->ParentTransactionRecord->ParentTransactionRecord->ParentTransactionRecord->Lsn, 1);
            VERIFY_ARE_EQUAL(chain->ParentTransactionRecord->ParentTransactionRecord->ParentTransactionRecord->ParentTransactionRecord, nullptr);

            invalidLogRecords_.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(TxMap_RandomNumberOfPendingAndCommitedTransactions_VerifyState)
    {
        TEST_TRACE_BEGIN("TxMap_RandomNumberOfPendingAndCommitedTransactions_VerifyState")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            
            DWORD tSeed = GetTickCount();
            Common::Random random(tSeed);
            Trace.WriteInfo(TraceComponent, "TxMap_RandomNumberOfPendingAndCommitedTransactions_VerifyState: Using Random Seed Number {0}", tSeed);

            int txCount = (random.Next() % 1000) + 100;

            KArray<BeginTransactionOperationLogRecord::SPtr> pendingTxs(allocator);
            KArray<BeginTransactionOperationLogRecord::SPtr> committedTxs(allocator);
            LONG64 lsn = 2;

            for (int i = 0; i < txCount; i++)
            {
                TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
                BeginTransactionOperationLogRecord::SPtr beginTx = CreateBeginTx(*transaction->Tx, lsn++, false);
                map->CreateTransaction(*beginTx);
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));

                if (random.Next() % 3 == 0)
                {
                    Trace.WriteInfo(TraceComponent, "Committing Tx {0}", transaction->Tx->TransactionId);
                    EndTransactionLogRecord::SPtr commit = CreateEndTx(*transaction->Tx, true, lsn++);
                    map->CompleteTransaction(*commit);
                    committedTxs.Append(beginTx);
                }
                else
                {
                    pendingTxs.Append(beginTx);
                }
            }

            VerifyPendingTxStats(*map, pendingTxs.Count());
            VerifyCompletedTxStats(*map, committedTxs.Count());

            invalidLogRecords_.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(TxMap_GetEarliestTx)
    {
        TEST_TRACE_BEGIN("TxMap_GetEarliestTx")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            
            DWORD tSeed = GetTickCount();
            Common::Random random(tSeed);
            Trace.WriteInfo(TraceComponent, "TxMap_GetEarliestTx: Using Random Seed Number {0}", tSeed);

            int txCount = (random.Next() % 1000) + 100;

            KArray<BeginTransactionOperationLogRecord::SPtr> pendingTxs(allocator);
            KArray<BeginTransactionOperationLogRecord::SPtr> committedTxs(allocator);
            LONG64 lsn = 2;

            for (int i = 0; i < txCount; i++)
            {
                TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
                BeginTransactionOperationLogRecord::SPtr beginTx = CreateBeginTx(*transaction->Tx, lsn++, false);
                map->CreateTransaction(*beginTx);
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));

                if (random.Next() % 3 == 0)
                {
                    Trace.WriteInfo(TraceComponent, "Committing Tx {0}", transaction->Tx->TransactionId);
                    EndTransactionLogRecord::SPtr commit = CreateEndTx(*transaction->Tx, true, lsn++);
                    map->CompleteTransaction(*commit);
                    committedTxs.Append(beginTx);
                }
                else
                {
                    pendingTxs.Append(beginTx);
                }
            }

            BeginTransactionOperationLogRecord::SPtr earliestPendingTx = map->GetEarliestPendingTransaction();
            if (pendingTxs.Count() > 0)
            {
                VERIFY_ARE_EQUAL(earliestPendingTx->Lsn, pendingTxs[0]->Lsn);
                LONG64 barrierLsn = earliestPendingTx->Lsn - 1;
                bool failedBarrierCheck = false;
                BeginTransactionOperationLogRecord::SPtr earliestPendingTxFailedBarrierCheck = map->GetEarliestPendingTransaction(barrierLsn, failedBarrierCheck);
                VERIFY_ARE_EQUAL(earliestPendingTxFailedBarrierCheck, nullptr);
                VERIFY_ARE_EQUAL(failedBarrierCheck, true);
            }
            else
            {
                VERIFY_ARE_EQUAL(earliestPendingTx, nullptr);
                LONG64 barrierLsn = 1;
                bool failedBarrierCheck = false;
                BeginTransactionOperationLogRecord::SPtr earliestPendingTxFailedBarrierCheck = map->GetEarliestPendingTransaction(barrierLsn, failedBarrierCheck);
                VERIFY_ARE_EQUAL(earliestPendingTxFailedBarrierCheck, nullptr);
                VERIFY_ARE_EQUAL(failedBarrierCheck, false);
            }

            invalidLogRecords_.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(TxMap_GetPendingRecordsAndTransactions)
    {
        TEST_TRACE_BEGIN("TxMap_GetPendingRecordsAndTransactions")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            
            DWORD tSeed = GetTickCount();
            Common::Random random(tSeed);
            Trace.WriteInfo(TraceComponent, "TxMap_GetPendingRecordsAndTransactions: Using Random Seed Number {0}", tSeed);

            int txCount = (random.Next() % 1000) + 100;

            KArray<BeginTransactionOperationLogRecord::SPtr> pendingTxs(allocator);
            KArray<BeginTransactionOperationLogRecord::SPtr> committedTxs(allocator);
            KArray<BeginTransactionOperationLogRecord::SPtr> oldTxs(allocator);
            LONG64 lsn = 2;
            ULONG64 olderThanPosition = (random.Next() % txCount);

            for (int i = 0; i < txCount; i++)
            {
                TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
                BeginTransactionOperationLogRecord::SPtr beginTx = CreateBeginTx(*transaction->Tx, lsn++, false);
                map->CreateTransaction(*beginTx);
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));
                map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));

                if (random.Next() % 3 == 0)
                {
                    Trace.WriteInfo(TraceComponent, "Committing Tx {0}", transaction->Tx->TransactionId);
                    EndTransactionLogRecord::SPtr commit = CreateEndTx(*transaction->Tx, true, lsn++);
                    map->CompleteTransaction(*commit);
                    committedTxs.Append(beginTx);
                }
                else
                {
                    pendingTxs.Append(beginTx);

                    if (beginTx->RecordPosition <= olderThanPosition)
                    {
                        oldTxs.Append(beginTx);
                    }
                }
            }

            KArray<TransactionLogRecord::SPtr> pendingRecords(allocator);
            map->GetPendingRecords(pendingRecords);
            VERIFY_ARE_EQUAL(pendingRecords.Count(), pendingTxs.Count());

            KArray<BeginTransactionOperationLogRecord::SPtr> pendingTransactions(allocator);
            map->GetPendingTransactions(pendingTransactions);
            VERIFY_ARE_EQUAL(pendingTransactions.Count(), pendingTxs.Count());

            KArray<BeginTransactionOperationLogRecord::SPtr> pendingTransactionsOlderThanRecordPosition(allocator);
            map->GetPendingTransactionsOlderThanPosition(olderThanPosition, pendingTransactionsOlderThanRecordPosition);
            VERIFY_ARE_EQUAL(pendingTransactionsOlderThanRecordPosition.Count(), oldTxs.Count());

            invalidLogRecords_.Reset();
        }
    }

    // This test case is to verify that we do not run into stack overflow while free-ing parent chain
    BOOST_AUTO_TEST_CASE(TxMap_10KOpsIn1Tx)
    {
        TEST_TRACE_BEGIN("TxMap_10KOpsIn1Tx")

        {
            TransactionMap::SPtr map = TransactionMap::Create(*prId_, allocator);
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            
            DWORD tSeed = GetTickCount();
            Common::Random random(tSeed);
            Trace.WriteInfo(TraceComponent, "TxMap_10KOpsIn1Tx: Using Random Seed Number {0}", tSeed);

            int operationCount = 10000;
            LONG64 lsn = 2;
           
            // Do this inside an indent to not keep any transaction objects alive
            {
                TestTransaction::SPtr transaction = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
                BeginTransactionOperationLogRecord::SPtr beginTx = CreateBeginTx(*transaction->Tx, lsn++, false);
                map->CreateTransaction(*beginTx);

                for (int i = 0; i < operationCount; i++)
                {
                    map->AddOperation(*CreateOperation(*transaction->Tx, lsn++));
                }

                Trace.WriteInfo(TraceComponent, "Committing Tx {0}", transaction->Tx->TransactionId);
                EndTransactionLogRecord::SPtr commit = CreateEndTx(*transaction->Tx, true, lsn++);
                map->CompleteTransaction(*commit);

                TestTransaction::SPtr transaction2 = TestTransaction::Create(*invalidLogRecords_, 1, 1, false, STATUS_SUCCESS, allocator);
                BeginTransactionOperationLogRecord::SPtr beginTx2 = CreateBeginTx(*transaction->Tx, lsn++, false);
                map->CreateTransaction(*beginTx2);
                EndTransactionLogRecord::SPtr commit2 = CreateEndTx(*transaction->Tx, true, lsn++);
                map->CompleteTransaction(*commit2);
            }

            map->RemoveStableTransactions(lsn);
            
            invalidLogRecords_.Reset();
        }
    }

    // TODO: Add false progress API test cases in the future

    BOOST_AUTO_TEST_SUITE_END()
}
