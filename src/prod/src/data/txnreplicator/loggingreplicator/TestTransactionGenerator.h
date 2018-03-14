// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestTransactionGenerator final
    {

    public:

        static TestTransaction::SPtr Create(
            __in INT numberOfOperations,
            __in bool isCommitted,
            __in KAllocator & allocator);

        static TestTransaction::SPtr Create(
            __in INT numberOfOperations,
            __in ULONG bufferCount,
            __in ULONG operationSize,
            __in bool isCommitted,
            __in KAllocator & allocator);

        static TestTransaction::SPtr Create(
            __in INT numberOfOperations,
            __in ULONG bufferCount,
            __in ULONG operationSize,
            __in bool isCommitted,
            __in NTSTATUS flushError,
            __in KAllocator & allocator);

        static KArray<Data::LogRecordLib::LogRecord::SPtr> InterleaveTransactions(
            __in KArray<TestTransaction::SPtr> const & transactionList,
            __in LONG64 startingLsn,
            __in int seed,
            __in KAllocator & allocator,
            __out LONG64 & lastLsn);
        
        static TestGroupCommitValidationResult InsertBarrier(
            __inout KArray<Data::LogRecordLib::LogRecord::SPtr> & transactionList,
            __in UINT committedTxCount,
            __in UINT abortedTxCount,
            __in UINT pendingTxCount,
            __in UINT opsPerTx,
            __in int seed,
            __in KAllocator & allocator);
    };
}
