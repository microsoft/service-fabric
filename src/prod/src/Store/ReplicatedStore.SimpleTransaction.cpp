// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("SimpleTransaction");

    ReplicatedStore::SimpleTransaction::SimpleTransaction(
        __in SimpleTransactionGroup & parent,
        std::shared_ptr<TransactionReplicator> const & txReplicator,
        Common::ActivityId const & activityId)
        : ReplicatedStore::TransactionBase(parent.ReplicatedStore, txReplicator, activityId)
        , parent_(parent)
        , parentRoot_(parent.CreateComponentRoot())
        , isReleased_(false)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: {1} SimpleTransaction::ctor",
            this->TraceId,
            static_cast<void*>(this));
    }

    ReplicatedStore::SimpleTransaction::~SimpleTransaction()
    {
        WriteNoise(
            TraceComponent, 
            "{0}: {1} SimpleTransaction::~dtor",
            this->TraceId,
            static_cast<void*>(this));

        this->OnDtor();
    }

    ErrorCode ReplicatedStore::SimpleTransaction::TryGetInnerTransaction(__out TransactionSPtr & txSPtr)
    {
        return parent_.TryGetInnerTransactionCallerHoldsLock(txSPtr);
    }

    bool ReplicatedStore::SimpleTransaction::TryReleaseInnerTransaction()
    {
        parent_.ReleaseInnerTransaction();

        return !isReleased_.exchange(true);
    }

    void ReplicatedStore::SimpleTransaction::OnRollback()
    {
        WriteInfo(
            TraceComponent, 
            "{0}: {1} SimpleTransaction::Rollback",
            this->TraceId,
            static_cast<void*>(this));

        parent_.Rollback(this->ActivityId);
    }

    ErrorCode ReplicatedStore::SimpleTransaction::AddReplicationOperation(ReplicationOperation && operation)
    {
        return parent_.AddReplicationOperation(move(operation), this->ActivityId);
    }

    AsyncOperationSPtr ReplicatedStore::SimpleTransaction::BeginCommit(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: {1} SimpleTransaction::BeginCommit",
            this->TraceId,
            static_cast<void*>(this));

        return AsyncOperation::CreateAndStart<ReplicateAsyncOperation<SimpleTransaction>>(
            *this,
            timeout,
            callback,
            root);
    }

    ErrorCode ReplicatedStore::SimpleTransaction::EndCommit(
        AsyncOperationSPtr const & operation,
        __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: {1} SimpleTransaction::EndCommit",
            this->TraceId,
            static_cast<void*>(this));

        auto error = ReplicateAsyncOperation<SimpleTransaction>::End(operation, commitSequenceNumber);

        if (error.IsSuccess())
        {
            // Real LSN comes from underlying transaction group
            //
            commitSequenceNumber = parent_.OperationLSN;
        }
        
        return error;
    }

    void ReplicatedStore::SimpleTransaction::AddToParent(AsyncOperationSPtr const & pendingReplication)
    {
        parent_.AddCommitAsyncOperation(this->ActivityId, pendingReplication);
    }

    void ReplicatedStore::SimpleTransaction::OnOperationFailed(ErrorCode const &)
    {
        parent_.Rollback(this->ActivityId);
    }
}
