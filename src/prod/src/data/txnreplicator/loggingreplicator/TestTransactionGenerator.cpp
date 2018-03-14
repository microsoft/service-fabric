// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

TestTransaction::SPtr TestTransactionGenerator::Create(
    __in INT numberOfOperations,
    __in bool isCommitted,
    __in KAllocator & allocator)
{
    return Create(
        numberOfOperations, 
        1, 
        1, 
        isCommitted, 
        allocator);
}

TestTransaction::SPtr TestTransactionGenerator::Create(
    __in INT numberOfOperations,
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in bool isCommitted,
    __in KAllocator & allocator)
{
    return Create(
        numberOfOperations,
        bufferCount,
        operationSize,
        isCommitted,
        STATUS_SUCCESS,
        allocator);
}

TestTransaction::SPtr TestTransactionGenerator::Create(
    __in INT numberOfOperations,
    __in ULONG bufferCount,
    __in ULONG operationSize,
    __in bool isCommitted,
    __in NTSTATUS flushError,
    __in KAllocator & allocator)
{
    InvalidLogRecords::SPtr invLogRecords = InvalidLogRecords::Create(allocator);

    if (numberOfOperations == 0)
    {
        CODING_ERROR_ASSERT(isCommitted);  // Cannot have a single operation transaction that is not committed

        TestTransaction::SPtr testTransaction = TestTransaction::Create(*invLogRecords, bufferCount, operationSize, true, flushError, allocator);
        return testTransaction;
    }

    TestTransaction::SPtr testTransaction = TestTransaction::Create(*invLogRecords, bufferCount, operationSize, false, flushError, allocator);
    for (INT i = 0; i < numberOfOperations; i++)
    {
        testTransaction->AddOperation(bufferCount, operationSize, flushError);
    }

    testTransaction->Commit(isCommitted);

    return testTransaction;
}

KArray<LogRecord::SPtr> TestTransactionGenerator::InterleaveTransactions(
    __in KArray<TestTransaction::SPtr> const & transactionList,
    __in LONG64 startingLsn,
    __in int seed,
    __in KAllocator & allocator,
    __out LONG64 & lastLsn)
{
    KArray<LogRecord::SPtr> logRecords(allocator);

    CODING_ERROR_ASSERT(transactionList.Count() >= 1);

    Common::Random random(seed);
    ULONG start = 0;
    ULONG iterCount = 0;
    ULONG totalRecordCount = 0;

    for (ULONG i = 0; i < transactionList.Count(); i++)
    {
        totalRecordCount += transactionList[i]->LogRecords.Count();
    }

    KArray<ULONG> indices(allocator, transactionList.Count());
    CODING_ERROR_ASSERT(indices.SetCount(transactionList.Count()) == TRUE);

    for (ULONG i = 0; i < transactionList.Count(); i++)
    {
        indices[i] = 0;
    }

    do
    {
        ULONG rndIndex = random.Next(0, static_cast<int>(indices.Count()));

        if (indices[rndIndex] < transactionList[rndIndex]->LogRecords.Count())
        {
            LONG64 lsn = startingLsn;
            transactionList[rndIndex]->LogRecords[indices[rndIndex]]->Lsn = lsn;
            transactionList[rndIndex]->LogRecords[indices[rndIndex]]->Psn = lsn;
            startingLsn += 1;

            logRecords.Append(transactionList[rndIndex]->LogRecords[indices[rndIndex]].RawPtr());
            indices[rndIndex] += 1;

            start += 1;
        }

        iterCount += 1;
    } while (start < totalRecordCount);

    lastLsn = startingLsn;

    return logRecords;
}

TestGroupCommitValidationResult TestTransactionGenerator::InsertBarrier(
    __inout KArray<LogRecord::SPtr> & transactionList,
    __in UINT committedTxCount,
    __in UINT abortedTxCount,
    __in UINT pendingTxCount,
    __in UINT opsPerTx,
    __in int seed,
    __in KAllocator & allocator)
{
    CODING_ERROR_ASSERT(transactionList.Count() >= 1);

    KArray<TestTransaction::SPtr> testTxList(allocator);

    InvalidLogRecords::SPtr invLogRecords = InvalidLogRecords::Create(allocator);

    for (ULONG i = 0; i < pendingTxCount; i++)
    {
        TestTransaction::SPtr testTx  = TestTransaction::Create(*invLogRecords, 1, 1, false, STATUS_SUCCESS, allocator);

        for (ULONG j = 0; j < opsPerTx; j++)
        {
            testTx->AddOperation();
        }

        testTxList.Append(testTx);
    }

    LONG64 lastLsn = transactionList[transactionList.Count() - 1]->Lsn;

    if (testTxList.Count() >= 1)
    {
        auto interleavedTxns = InterleaveTransactions(testTxList, transactionList[transactionList.Count() - 1]->Lsn, seed, allocator, lastLsn);
        for (ULONG count = 0; count < interleavedTxns.Count(); count++)
        {
            transactionList.Append(interleavedTxns[count].RawPtr());
        }
    }

    BarrierLogRecord::SPtr barrier = BarrierLogRecord::Create(LogRecordType::Enum::Barrier, 2, -1, *invLogRecords->Inv_PhysicalLogRecord, allocator);
    barrier->Lsn = lastLsn;
    barrier->CompletedFlush(STATUS_SUCCESS);

    transactionList.Append(barrier.RawPtr());

    TestGroupCommitValidationResult result(lastLsn, pendingTxCount, committedTxCount, abortedTxCount);

    return result;
}
