// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator 
{
    //
    // Represents a user transaction object that contains a series of operations that are all done atomically
    // Any API in this class can throw the following exceptions:
    //
    // Transaction Retriable Error Codes:- Where the operation can be retried on the same transaction or new transaction
    //      SF_STATUS_RECONFIGURATION_PENDING
    //      SF_STATUS_REPLICATION_QUEUE_FULL
    //      SF_STATUS_NO_WRITE_QUORUM
    //      SF_STATUS_SERVICE_TOO_BUSY - Thrown when write throttling is necessary
    //      SF_STATUS_TIMEOUT
    //      STATUS_CANCELLED
    //
    // Replica Retriable Error Codes:- Where the operation can be retried on a new transaction, but NOT the same transaction object
    //      SF_STATUS_TRANSACTION_ABORTED - System aborted 
    //
    // Non-Retriable Error Codes:
    //      SF_STATUS_NOT_PRIMARY
    //      SF_STATUS_OBJECT_CLOSED
    // 
    // Fatal Error Codes(Usage Bug)
    //      SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED - If API races with another pending API
    //      SF_STATUS_TRANSACTION_NOT_ACTIVE - Already committed or aborted or user is trying to perform operations on a non-primary transaction
    //
    interface ITransaction
    {
        K_SHARED_INTERFACE(ITransaction)

    public:

        virtual ktl::Awaitable<NTSTATUS> CommitAsync(
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

        // This overload implies timeout is INFINITE
        virtual ktl::Awaitable<NTSTATUS> CommitAsync() noexcept = 0;

        virtual NTSTATUS Abort() noexcept = 0;
    };
}
